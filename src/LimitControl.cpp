// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "LimitControl.h"
#include "MessageOutput.h"
#include "ShellyClient.h"
#include <cfloat>

LimitControlClass LimitControl;

LimitControlClass::LimitControlClass()
    : _loopTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&LimitControlClass::loop, this))
    , _shellyClientData(ShellyClient.getShellyData())
    , _invLimitAbsolute(0)
{
    _intervalPro3em = 5000;
    _intervalPlugS = 5000;
}

void LimitControlClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void LimitControlClass::loop()
{
    const CONFIG_T& config = Configuration.get();
    if (!(config.Shelly.ShellyEnable && config.Shelly.LimitEnable)) {
        _intervalPro3em = 5000;
        _intervalPlugS = 5000;
        return;
    }

    if (millis() - _lastLimitSend < 10 * TASK_SECOND) {
        return;
    }
    _lastLimitSend = millis();

#if 0
    // TODO MQTT funktioniert, hier nicht Frontend nicht
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        if (inv->SystemConfigPara()->getLastUpdate() > 0) {
            uint16_t maxpower = inv->DevInfo()->getMaxPower();
            if (maxpower > 0) {
                _invLimitAbsolute = inv->SystemConfigPara()->getLimitPercent() * maxpower / 100;
            }
        }
    }
#endif

    CalculateLimit();
}

void LimitControlClass::CalculateLimit()
{
    const CONFIG_T& config = Configuration.get();

    float gridPower = _shellyClientData.GetFactoredValue(ShellyClientType_t::Pro3EM, _intervalPro3em);
    float generatedPower = _shellyClientData.GetFactoredValue(ShellyClientType_t::PlugS, _intervalPlugS);
    MessageOutput.printf("LimitControlClass::LimitControlClass grid:%f, generatedPower:%f \r\n", gridPower, generatedPower);

    /*
    _shellyClientMqtt.Update(ShellyClientMqttType_t::Pro3EMPower, gridPower);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::Pro3EMTime, _shellyClientData.GetMinMaxTime(ShellyClientType_t::Pro3EM) / 1000);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::PlugSPower, generatedPower);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::PlugSTime, _shellyClientData.GetMinMaxTime(ShellyClientType_t::PlugS) / 1000);
*/
    float limit = FLT_MIN;
    float border = 5;

    if (gridPower > config.Shelly.TargetValue + border) {
        // increase: iterate to limit
        limit = abs(gridPower - config.Shelly.TargetValue);
        limit *= 0.75; // IncreaseFactor(limit);
        limit += _actLimit;
        limit = static_cast<int>(limit + 0.5) + 0.1;

        // Debug("i");
    } else if (gridPower < config.Shelly.TargetValue - border - 50) {
        // decrease: set new limit
        //_increaseCnt = 0;
        /// limit = abs(static_cast<int32_t>(config.Shelly.MinPower) - config.Shelly.TargetValue);
        limit = generatedPower - abs(gridPower - config.Shelly.TargetValue) * 0.9f;

        limit = static_cast<int>(limit + 0.5) - 0.5;

        //_shellyClientData.SetLastValue(config.Shelly.TargetValue);

        // Debug("D");
    } else if (gridPower < config.Shelly.TargetValue - border) {
        // decrease: set new limit
        //_increaseCnt = 0;
        /*if (abs(gridPower - generatedPower) > 50) {
            limit = generatedPower - abs(gridPower - config.Shelly.TargetValue) * 0.9f;
            limit = static_cast<int>(limit + 0.5) - 0.2;
        } else*/
        {
            limit = abs(gridPower - config.Shelly.TargetValue);
            limit *= 0.8; // DecreaseFactor(limit);
            limit = _actLimit - limit;
            limit = static_cast<int>(limit + 0.5) - 0.1;
        }
        // border = min(1, 2);
        //_shellyClientData.SetLastValue(config.Shelly.TargetValue - border);
        // Debug("d");
    } else {
        //_increaseCnt = 0;
        // Debug("e");
    }

    MessageOutput.printf("LimitControlClass::LimitControlClass %f \r\n", limit);
    return;

    if (limit != FLT_MIN) {
        bool increase = limit > _actLimit;
        /*if (_increaseCnt > 5) {
            limit = config.Shelly.MaxPower;
        }*/

        SendLimitResult_t result = SendLimit(limit, generatedPower);
        switch (result) {
        case SendLimitResult_t::NoInverter:
            // Debug("N");
            break;
        case SendLimitResult_t::SendOk:
            // Debug(limit);
            // Debug("S");
            if (increase) {
                //_increaseCnt++;
            }
            break;
        case SendLimitResult_t::CommandPending:
            // Debug("T");
            break;
        case SendLimitResult_t::Similar:
            // Debug("~");
            break;
        default:
            break;
        };
    }

    //_Pro3EM.MaxInterval = 500;
    //_PlugS.MaxInterval = 500;

    /*
    if (_shellyClientData.GetMaxValue(ShellyClientType_t::PlugS) < config.Shelly.MinPower) {
        _Pro3EM.MaxInterval = 10000;
        _PlugS.MaxInterval = 10000;
    }*/
}

SendLimitResult_t LimitControlClass::SendLimit(float limit, float generatedPower)
{
    auto inv = Hoymiles.getInverterByPos(0);
    if (inv == nullptr || !inv->isReachable()) {
        return SendLimitResult_t::NoInverter;
    }

    const CONFIG_T& config = Configuration.get();

    if (config.Shelly.MinPower > config.Shelly.TargetValue && limit < config.Shelly.MinPower - config.Shelly.TargetValue) {
        limit = config.Shelly.MinPower - config.Shelly.TargetValue;
    }

    if (limit > config.Shelly.MaxPower) {
        limit = config.Shelly.MaxPower;
    }

    if (_actLimit == limit || abs(_actLimit - limit) < 15) { // if lower than 10, more update are send
        return SendLimitResult_t::Similar;
    }

    _actLimit = limit;

    float correctPanelCnt = 1.0; // 0.75 => 3 of 4 pannels installed
    if (correctPanelCnt != 1) {
        limit /= correctPanelCnt;
        if (limit > config.Shelly.MaxPower) {
            limit = config.Shelly.MaxPower;
        }
    }
    inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent);
    _lastLimitSend = millis();

    return SendLimitResult_t::SendOk;
}

#if 0
float ShellyClientClass::IncreaseFactor(float diff)
{
    static struct {
        float p1x;
        float p1y;
        float p2x;
        float p2y;
    } f = { 10.0f, 0.5f, 150.0f, 0.7f };
    static float m = (f.p2y - f.p1y) / (f.p2x - f.p1x);
    static float n = f.p1y - m * f.p1x;
    if (diff <= f.p1x) {
        return f.p1y;
    }
    if (diff >= f.p2x) {
        return f.p2y;
    }
    return (diff * m + n);
}

float ShellyClientClass::DecreaseFactor(float diff)
{
    static struct {
        float p1x;
        float p1y;
        float p2x;
        float p2y;
    } f = { 10.0f, 0.5f, 150.0f, 0.7f };
    static float m = (f.p2y - f.p1y) / (f.p2x - f.p1x);
    static float n = f.p1y - m * f.p1x;
    if (diff <= f.p1x) {
        return f.p1y;
    }
    if (diff >= f.p2x) {
        return f.p2y;
    }
    return (diff * m + n);
}

uint32_t ShellyClientClass::getLastUpdate()
{
    uint32_t pro = _shellyClientData.GetUpdateTime(ShellyClientType_t::Pro3EM);
    uint32_t plugs = _shellyClientData.GetUpdateTime(ShellyClientType_t::PlugS);

    return pro > plugs ? pro : plugs;
}

void ShellyClientClass::Debug(const char* text)
{
    if (Configuration.get().Shelly.DebugEnable) {
        if (_Debug.length() > 80) {
            _Debug.clear();
        }

        _Debug += text;
    }
}

void ShellyClientClass::Debug(float number)
{
    if (Configuration.get().Shelly.DebugEnable) {
        _Debug += String(number, 0);
    }
}

String ShellyClientClass::getDebug()
{
    // String info = String(_Pro3EMData.GetCircularBufferTime() / 1000) + ", " + String(_PlugSData.GetCircularBufferTime() / 1000);
    // return info;
    String debug = _Debug;
    _Debug = "";
    return debug;
}
#endif