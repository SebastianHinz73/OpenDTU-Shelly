// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClient.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "SunPosition.h"
#include <Arduino.h>
#include <Hoymiles.h>
#include <cfloat>

ShellyClientClass ShellyClient;

ShellyClientClass::ShellyClientClass() 
   : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&ShellyClientClass::loop, this))
{
}

void ShellyClientClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
    _Pro3EM.ShellyType = ShellyClientType_t::Pro3EM;
    _PlugS.ShellyType = ShellyClientType_t::PlugS;

    _actLimit = 0;
    _increaseCnt = 0;
}

void ShellyClientClass::loop()
{
    while (WiFi.status() != WL_CONNECTED) {
        return;
    }
    if (!SunPosition.isDayPeriod()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    HandleWebsocket(_Pro3EM, config.Shelly.Hostname_Pro3EM, ShellyClientClass::EventsPro3EM);
    HandleWebsocket(_PlugS, config.Shelly.Hostname_PlugS, ShellyClientClass::EventsPlugS);

    _shellyClientMqtt.Loop();
}

void ShellyClientClass::HandleWebsocket(WebSocketData& data, const char* hostname, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent)
{
    const CONFIG_T& config = Configuration.get();

    bool bDelete = data.Host.compare(hostname) != 0; // hostname changed in configuration
    bDelete |= strlen(hostname) == 0; // IP deleted in configuration
    bDelete |= !config.Shelly.ShellyEnable; // shelly disabled

    bool bNew = bDelete && strlen(hostname) > 0; // deleted and new hostname valid
    bNew &= config.Shelly.ShellyEnable; // && shelly enable

    if (bDelete && data.Client != nullptr) {
        MessageOutput.printf("Delete WebSocketClient. %s\r\n", data.Host.c_str());
        delete data.Client;
        data.Client = nullptr;
        data.Host = "";
    }

    if (bNew && data.Client == nullptr) {
        MessageOutput.printf("Add WebSocketClient %s\r\n", hostname);
        data.Client = new WebSocketsClient();
        data.Client->begin(hostname, 80, "/rpc");
        data.Client->onEvent(cbEvent);
        data.Client->setReconnectInterval(2000);
        data.Host = hostname;
        data.LastTime = millis();
    }

    if (data.Connected && (millis() - data.LastTime > data.MaxInterval)) {
        data.LastTime = millis();
        // MessageOutput.printf("SEND %s#####\r\n", data.ShellyType == ShellyClientType_t::Pro3EM ? "Pro" : "PlugS");
        data.Client->sendTXT("{\"id\":2, \"src\":\"user_1\", \"method\":\"Shelly.GetStatus\"}");
    }

    if (data.Client != nullptr) {
        data.Client->loop();
    }
}

void ShellyClientClass::EventsPro3EM(WStype_t type, uint8_t* payload, size_t length)
{
    ShellyClient.Events(ShellyClient._Pro3EM, type, payload, length);
}

void ShellyClientClass::EventsPlugS(WStype_t type, uint8_t* payload, size_t length)
{
    ShellyClient.Events(ShellyClient._PlugS, type, payload, length);
}

void ShellyClientClass::Events(WebSocketData& data, WStype_t type, uint8_t* payload, size_t length)
{
    switch (type) {
    case WStype_DISCONNECTED:
        MessageOutput.printf("[WSc] Disconnected!\r\n");
        data.Connected = false;
        break;
    case WStype_CONNECTED:
        MessageOutput.printf("[WSc] Connected to url: %s\r\n", payload);
        data.Connected = true;
        break;
    case WStype_TEXT:
        // MessageOutput.printf("[WSc] get text: %s\r\n", payload);
        {
            JsonDocument root;
            const DeserializationError error = deserializeJson(root, payload);

            if (error) {
                MessageOutput.printf("error %s\r\n", payload);
                return;
            }

            if (data.ShellyType == ShellyClientType_t::Pro3EM) {
                if (ParseJSON(data, root, "params", "em:0", "total_act_power") || // Notify Status
                    ParseJSON(data, root, "result", "em:0", "total_act_power")) { // Result of manual trigger (Shelly.GetStatus)
                    _shellyClientData.Update(data.ShellyType, data.LastMeasurement);
                    SetLimit();
                }
            } else {
                if (ParseJSON(data, root, "result", "switch:0", "apower")) {
                    _shellyClientData.Update(data.ShellyType, data.LastMeasurement);
                    SetLimit();
                }
            }
        }
        break;
    case WStype_BIN:
        MessageOutput.printf("[WSc] get binary length: %u\r\n", length);
        break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
        break;
    }
}

bool ShellyClientClass::ParseJSON(WebSocketData& data, JsonDocument& root, const char* name1, const char* name2, const char* name3)
{
    JsonObject n1 = root[name1];
    if (n1[name2].is<JsonObject>()) {
        JsonObject n2 = n1[name2];
        if (n2[name3].is<double_t>()) {
            data.LastMeasurement = n2[name3];
            data.LastTime = millis();
            return true;
        }
    }
    return false;
}

void ShellyClientClass::SetLimit()
{
    const CONFIG_T& config = Configuration.get();
    if (!(config.Shelly.ShellyEnable && config.Shelly.LimitEnable)) {
        _Pro3EM.MaxInterval = 5000;
        _PlugS.MaxInterval = 5000;
        return;
    }

    float gridPower = _shellyClientData.GetFactoredValue(ShellyClientType_t::Pro3EM);
    float generatedPower = _shellyClientData.GetFactoredValue(ShellyClientType_t::PlugS);

    _shellyClientMqtt.Update(ShellyClientMqttType_t::Pro3EMPower, gridPower);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::Pro3EMTime, _shellyClientData.GetMinMaxTime(ShellyClientType_t::Pro3EM) / 1000);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::PlugSPower, generatedPower);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::PlugSTime, _shellyClientData.GetMinMaxTime(ShellyClientType_t::PlugS) / 1000);

    float limit = FLT_MIN;
    float border = 5;

    if (gridPower > config.Shelly.TargetValue + border) {
        // increase: iterate to limit
        limit = abs(gridPower - config.Shelly.TargetValue);
        limit *= IncreaseFactor(limit);
        limit += _actLimit;
        limit = static_cast<int>(limit + 0.5) + 0.1;

        Debug("i");
    } else if (gridPower < config.Shelly.TargetValue - border - 50) {
        // decrease: set new limit
        _increaseCnt = 0;
        /// limit = abs(static_cast<int32_t>(config.Shelly.MinPower) - config.Shelly.TargetValue);
        limit = generatedPower - abs(gridPower - config.Shelly.TargetValue) * 0.9f;

        limit = static_cast<int>(limit + 0.5) - 0.5;

        _shellyClientData.SetLastValue(config.Shelly.TargetValue);

        Debug("D");
    } else if (gridPower < config.Shelly.TargetValue - border) {
        // decrease: set new limit
        _increaseCnt = 0;
        /*if (abs(gridPower - generatedPower) > 50) {
            limit = generatedPower - abs(gridPower - config.Shelly.TargetValue) * 0.9f;
            limit = static_cast<int>(limit + 0.5) - 0.2;
        } else*/
        {
            limit = abs(gridPower - config.Shelly.TargetValue);
            limit *= DecreaseFactor(limit);
            limit = _actLimit - limit;
            limit = static_cast<int>(limit + 0.5) - 0.1;
        }
        // border = min(1, 2);
        //_shellyClientData.SetLastValue(config.Shelly.TargetValue - border);
        Debug("d");
    } else {
        _increaseCnt = 0;
        Debug("e");
    }

    if (limit != FLT_MIN) {
        bool increase = limit > _actLimit;
        if (_increaseCnt > 5) {
            limit = config.Shelly.MaxPower;
        }

        SendLimitResult_t result = SendLimit(limit, generatedPower);
        switch (result) {
        case SendLimitResult_t::NoInverter:
            Debug("N");
            break;
        case SendLimitResult_t::SendOk:
            Debug(limit);
            Debug("S");
            if (increase) {
                _increaseCnt++;
            }
            break;
        case SendLimitResult_t::CommandPending:
            Debug("T");
            break;
        case SendLimitResult_t::Similar:
            Debug("~");
            break;
        default:
            break;
        };
    }

    _Pro3EM.MaxInterval = 500;
    _PlugS.MaxInterval = 500;

    if (_shellyClientData.GetMaxValue(ShellyClientType_t::PlugS) < config.Shelly.MinPower) {
        _Pro3EM.MaxInterval = 10000;
        _PlugS.MaxInterval = 10000;
    }
}

SendLimitResult_t ShellyClientClass::SendLimit(float limit, float generatedPower)
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

    if (_actLimit == limit || abs(_actLimit - limit) < 7) { // if lower than 10, more update are send
        return SendLimitResult_t::Similar;
    }

    float lastCommand = millis() - _lastCommandTrigger;
    if (lastCommand < _shellyClientData.GetMinMaxTime(ShellyClientType_t::Pro3EM) + 1000) {
        return SendLimitResult_t::CommandPending;
    }

    _lastCommandTrigger = millis();

    _actLimit = limit;

    float correctPanelCnt = 1.0; // 0.75 => 3 of 4 pannels installed
    if (correctPanelCnt != 1) {
        limit /= correctPanelCnt;
        if (limit > config.Shelly.MaxPower) {
            limit = config.Shelly.MaxPower;
        }
    }
    inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent);
    _shellyClientMqtt.Update(ShellyClientMqttType_t::Limit, limit);

    return SendLimitResult_t::SendOk;
}

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
