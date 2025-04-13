// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "LimitControl.h"
#include "MessageOutput.h"
#include "ShellyClient.h"
#include <Hoymiles.h>
#include <cfloat>

LimitControlClass LimitControl;

LimitControlClass::LimitControlClass()
    : _loopTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&LimitControlClass::loop, this))
    , _shellyClientData(ShellyClient.getShellyData())
    , _invLimitAbsolute(0)
{
    _intervalPro3em = 20000;
    _intervalPlugS = 20000;
}

void LimitControlClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void LimitControlClass::loop()
{
    float valueMax = _shellyClientData.GetMaxValue(RamDataType_t::Pro3EM, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::Pro3EM_Max, valueMax);

    float valueMin = _shellyClientData.GetMinValue(RamDataType_t::Pro3EM, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::Pro3EM_Min, valueMin);
    _debugPro3em = "[" + String(static_cast<int>(valueMin)) + "," + String(static_cast<int>(valueMax)) + "]";

    valueMax = _shellyClientData.GetMaxValue(RamDataType_t::PlugS, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::PlugS_Max, valueMax);

    valueMin = _shellyClientData.GetMinValue(RamDataType_t::PlugS, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::PlugS_Min, valueMin);
    _debugPlugS = "[" + String(static_cast<int>(valueMin)) + "," + String(static_cast<int>(valueMax)) + "]";

    const CONFIG_T& config = Configuration.get();
    if (!(config.Shelly.ShellyEnable && config.Shelly.LimitEnable)) {
        _intervalPro3em = 20000; // 5000;
        _intervalPlugS = 20000; // 5000;
        return;
    }

    auto inv = Hoymiles.getInverterByPos(0);
    bool bInv = inv != nullptr && inv->isReachable();
    if (!bInv) {
        _shellyClientData.Update(ShellyClientDataType_t::CalulatedLimit, "!inv->isReachable()");
    } else {
        FetchChannelPower(inv);
    }

    float limit = 0;
    if (CalculateLimit(limit) && bInv && millis() - _lastLimitSend > 10 * TASK_SECOND) {
        if (inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent)) {
            _actLimit = limit;
            _lastLimitSend = millis();
            _shellyClientData.Update(RamDataType_t::Limit, limit);
        }
    }
}

bool LimitControlClass::CalculateLimit(float& limit)
{
    const CONFIG_T& config = Configuration.get();

    float gridPower = _shellyClientData.GetFactoredValue(RamDataType_t::Pro3EM, _intervalPro3em);
    float generatedPower = _shellyClientData.GetFactoredValue(RamDataType_t::PlugS, _intervalPlugS);
    _debugPro3em += String(static_cast<int>(gridPower)) + ", " + String(static_cast<int>(_intervalPro3em / 1000)) + " ";
    _debugPlugS += String(static_cast<int>(generatedPower)) + ", " + String(static_cast<int>(_intervalPlugS / 1000)) + " ";

    limit = -FLT_MAX;
    float border = 10;

    if (gridPower > config.Shelly.TargetValue + border) {
        limit = Increase(_dataIncrease, gridPower);
    } else if (gridPower < config.Shelly.TargetValue - border) {
        limit = Decrease(_dataDecrease, gridPower, generatedPower);
    } else {
        limit = Optimize(_dataOptimize);
    }

    _shellyClientData.Update(RamDataType_t::Limit, _actLimit); // update

    _shellyClientData.Update(ShellyClientDataType_t::Pro3EM, _debugPro3em);
    _shellyClientData.Update(ShellyClientDataType_t::PlugS, _debugPlugS);

    if (limit == -FLT_MAX) {
        _shellyClientData.Update(RamDataType_t::CalulatedLimit, _shellyClientData.GetActValue(RamDataType_t::CalulatedLimit)); // update
        return false;
    }
    _shellyClientData.Update(RamDataType_t::CalulatedLimit, limit);

    return true;
}

float LimitControlClass::Increase(CalcLimitFunctionData_t& context, float gridPower)
{
    const CONFIG_T& config = Configuration.get();
    HandleCnt(context);

    float limit = abs(gridPower - config.Shelly.TargetValue);
    limit *= 0.75;
    limit += _actLimit;

    return CheckBoundary(limit);
}

float LimitControlClass::Decrease(CalcLimitFunctionData_t& context, float gridPower, float generatedPower)
{
    const CONFIG_T& config = Configuration.get();
    HandleCnt(context);

    float limit;

    if (gridPower < config.Shelly.TargetValue - 50) {
        // decrease: set new limit
        limit = generatedPower - abs(gridPower - config.Shelly.TargetValue) * 0.9f;
        limit = CorrectChannelPower(limit);
    } else {
        limit = abs(gridPower - config.Shelly.TargetValue);
        limit *= 0.8;
        limit = _actLimit - limit;
    }

    return CheckBoundary(limit);
}

float LimitControlClass::Optimize(CalcLimitFunctionData_t& context)
{
    HandleCnt(context);

    float limit = -FLT_MAX;
    return limit;
}

void LimitControlClass::HandleCnt(CalcLimitFunctionData_t& context)
{
    context._consecutiveCnt++;

    if (&context != &_dataDecrease) {
        _dataDecrease._consecutiveCnt = 0;
    }
    if (&context != &_dataIncrease) {
        _dataIncrease._consecutiveCnt = 0;
    }
    if (&context != &_dataOptimize) {
        _dataOptimize._consecutiveCnt = 0;
    }
}

void LimitControlClass::FetchChannelPower(std::shared_ptr<InverterAbstract> inv)
{
    _channelCnt = 0;
    String limit = "(";
    for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
        if (inv->Statistics()->getStringMaxPower(c) > 0) {
            auto power = inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
            _channelPower[c] = power;
            _channelCnt++;
            limit += String(static_cast<int>(power)) + ",";
        }
    }
    limit += ")";
    _shellyClientData.Update(ShellyClientDataType_t::CalulatedLimit, limit);
}

float LimitControlClass::CorrectChannelPower(float neededPower)
{
    // its possible that the pannels generated different power (e.g. east/west)
    // the inverter cuts all pannels to the same value (e.g. for 4 pannel to 1/4)
    // Used on decrease: function calculates the optimal limit to fulfill needed power

    float max = 0;
    float min = FLT_MAX;
    for (int i = 0; i < _channelCnt; i++) {
        min = _min(min, _channelPower[i]);
        max = _max(max, _channelPower[i]);
    }

    // all pannels generates more than needed power
    // e.g. four pannels (100W, 100W, 300W, 300W), neededPower 200W;
    if (neededPower / _channelCnt < _channelCnt * min) {
        return neededPower;
    }

    // e.g. four pannels (100W, 100W, 300W, 300W), neededPower 500W -> (100W, 100W, 150W, 150W) neededPower = 4*150
    for (float act = min; act <= max; act++) {
        float sum = 0;
        for (int i = 0; i < _channelCnt; i++) {
            sum += _min(act, _channelPower[i]);
        }
        if (sum > neededPower) // sum of channels > needed/expected power
        {
            return _channelCnt * act;
        }
    }

    // all pannels generates less than needed power
    // e.g. four pannels (10W, 10W, 100W, 100W), neededPower 500W -> channelPower = neededPower
    return neededPower;
}

float LimitControlClass::CheckBoundary(float limit)
{
    const CONFIG_T& config = Configuration.get();

    if (config.Shelly.MinPower > config.Shelly.TargetValue && limit < config.Shelly.MinPower - config.Shelly.TargetValue) {
        limit = config.Shelly.MinPower - config.Shelly.TargetValue;
    }

    if (limit > config.Shelly.MaxPower) {
        limit = config.Shelly.MaxPower;
    }

    if (_actLimit == limit || abs(_actLimit - limit) < 15) { // if lower than 10, more update are send
        return -FLT_MAX;
    }

    return limit;
}
