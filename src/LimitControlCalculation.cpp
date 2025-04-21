// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */
#include "LimitControlCalculation.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include <cfloat>

LimitControlCalculation::LimitControlCalculation(IShellyClientData& shellyClientData, IShellyWrapper& shellyWrapper)
    : _shellyClientData(shellyClientData)
    , _shellyWrapper(shellyWrapper)
{
    _intervalPro3em = 20000;
    _intervalPlugS = 20000;
}

void LimitControlCalculation::loop()
{
    float valueMax = _shellyClientData.GetMaxValue(RamDataType_t::Pro3EM, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::Pro3EM_Max, valueMax);

    float valueMin = _shellyClientData.GetMinValue(RamDataType_t::Pro3EM, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::Pro3EM_Min, valueMin);
    _debugPro3em = "[" + std::to_string(static_cast<int>(valueMin)) + "," + std::to_string(static_cast<int>(valueMax)) + "]";

    valueMax = _shellyClientData.GetMaxValue(RamDataType_t::PlugS, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::PlugS_Max, valueMax);

    valueMin = _shellyClientData.GetMinValue(RamDataType_t::PlugS, _intervalPro3em);
    _shellyClientData.Update(RamDataType_t::PlugS_Min, valueMin);
    _debugPlugS = "[" + std::to_string(static_cast<int>(valueMin)) + "," + std::to_string(static_cast<int>(valueMax)) + "]";

    const CONFIG_T& config = Configuration.get();
    if (!(config.Shelly.ShellyEnable && config.Shelly.LimitEnable)) {
        _intervalPro3em = 20000; // 5000;
        _intervalPlugS = 20000; // 5000;
        return;
    }

    MessageOutput.printf("calc %d Test\r\n", 123);

    bool bInv = _shellyWrapper.isReachable();
    if (!bInv) {
        _shellyClientData.Update(ShellyClientDataType_t::CalulatedLimit, "!inv->isReachable()");
    } else {
        _channelCnt = _shellyWrapper.fetchChannelPower(_channelPower);
    }

    float limit = 0;
    if (CalculateLimit(limit) && bInv && _shellyWrapper.millis() - _lastLimitSend > 10 * TASK_SECOND) {

        if (_shellyWrapper.sendLimit(limit)) {
            _actLimit = limit;
            _lastLimitSend = _shellyWrapper.millis();
            _shellyClientData.Update(RamDataType_t::Limit, limit);
        }
    }
}

bool LimitControlCalculation::CalculateLimit(float& limit)
{
    const CONFIG_T& config = Configuration.get();

    float gridPower = _shellyClientData.GetFactoredValue(RamDataType_t::Pro3EM, _intervalPro3em);
    float generatedPower = _shellyClientData.GetFactoredValue(RamDataType_t::PlugS, _intervalPlugS);
    _debugPro3em += std::to_string(static_cast<int>(gridPower)) + ", " + std::to_string(static_cast<int>(_intervalPro3em / 1000)) + " ";
    _debugPlugS += std::to_string(static_cast<int>(generatedPower)) + ", " + std::to_string(static_cast<int>(_intervalPlugS / 1000)) + " ";

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

float LimitControlCalculation::Increase(CalcLimitFunctionData_t& context, float gridPower)
{
    const CONFIG_T& config = Configuration.get();
    HandleCnt(context);

    float limit = abs(gridPower - config.Shelly.TargetValue);
    limit *= 0.75;
    limit += _actLimit;

    return CheckBoundary(limit);
}

float LimitControlCalculation::Decrease(CalcLimitFunctionData_t& context, float gridPower, float generatedPower)
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

float LimitControlCalculation::Optimize(CalcLimitFunctionData_t& context)
{
    HandleCnt(context);

    float limit = -FLT_MAX;
    return limit;
}

void LimitControlCalculation::HandleCnt(CalcLimitFunctionData_t& context)
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

float LimitControlCalculation::CorrectChannelPower(float neededPower)
{
    // its possible that the pannels generated different power (e.g. east/west)
    // the inverter cuts all pannels to the same value (e.g. for 4 pannel to 1/4)
    // Used on decrease: function calculates the optimal limit to fulfill needed power

    float fl_max = 0;
    float fl_min = FLT_MAX;
    for (int i = 0; i < _channelCnt; i++) {
        fl_min = std::min(fl_min, _channelPower[i]);
        fl_max = std::max(fl_max, _channelPower[i]);
    }

    // all pannels generates more than needed power
    // e.g. four pannels (100W, 100W, 300W, 300W), neededPower 200W;
    if (neededPower / _channelCnt < _channelCnt * fl_min) {
        return neededPower;
    }

    // e.g. four pannels (100W, 100W, 300W, 300W), neededPower 500W -> (100W, 100W, 150W, 150W) neededPower = 4*150
    for (float act = fl_min; act <= fl_max; act++) {
        float sum = 0;
        for (int i = 0; i < _channelCnt; i++) {
            sum += std::min(act, _channelPower[i]);
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

float LimitControlCalculation::CheckBoundary(float limit)
{
    const CONFIG_T& config = Configuration.get();
    int32_t minPower = static_cast<int32_t>(config.Shelly.MinPower);

    if (minPower > config.Shelly.TargetValue && limit < minPower - config.Shelly.TargetValue) {
        limit = minPower - config.Shelly.TargetValue;
    }

    if (limit > config.Shelly.MaxPower) {
        limit = config.Shelly.MaxPower;
    }

    if (_actLimit == limit || abs(_actLimit - limit) < 15) { // if lower than 10, more update are send
        return -FLT_MAX;
    }

    return limit;
}
