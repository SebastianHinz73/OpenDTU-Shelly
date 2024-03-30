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

void ShellyClientClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&ShellyClientClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
    _Pro3EM.IsPro3EM = true;
    _PlugS.IsPro3EM = false;
    _Pro3EMData.Init(true);
    _PlugSData.Init(false);

    _actLimit = 0;
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

    int pollInterval = SetLimit();
    HandleWebsocket(_Pro3EM, config.Shelly.Hostname_Pro3EM, pollInterval, ShellyClientClass::EventsPro3EM);
    HandleWebsocket(_PlugS, config.Shelly.Hostname_PlugS, pollInterval, ShellyClientClass::EventsPlugS);
}

void ShellyClientClass::HandleWebsocket(WebSocketData& data, const char* hostname, int poll_intervall, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent)
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

    if (data.Connected && (millis() - data.LastTime > poll_intervall)) {
        data.LastTime = millis();
        // MessageOutput.printf("SEND %s#####\r\n", data.IsPro3EM ? "Pro" : "PlugS");
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
            DynamicJsonDocument root(2048); // TODO check payload < 1024
            const DeserializationError error = deserializeJson(root, payload);

            if (error) {
                MessageOutput.printf("error %s\r\n", payload);
                return;
            }

            if (data.IsPro3EM) {
                if (JSONPro3EM(data, root, "params", "em:0", "total_act_power") || JSONPro3EM(data, root, "result", "em:0", "total_act_power")) {
                    _Pro3EMData.Update(data.LastMeasurement);
                }
            } else {
                if (JSONPro3EM(data, root, "result", "switch:0", "apower")) {
                    _PlugSData.Update(data.LastMeasurement);
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

bool ShellyClientClass::JSONPro3EM(WebSocketData& data, DynamicJsonDocument& root, const char* name1, const char* name2, const char* name3)
{
    JsonObject n1 = root[name1];
    if (n1.containsKey(name2)) {
        JsonObject n2 = n1[name2];
        if (n2.containsKey(name3)) {
            data.LastMeasurement = n2[name3];
            data.LastTime = millis();
            return true;
        }
    }
    return false;
}

int ShellyClientClass::SetLimit()
{
    const CONFIG_T& config = Configuration.get();
    if (!(config.Shelly.ShellyEnable && config.Shelly.LimitEnable)) {
        return 5000;
    }

    float gridPower = _Pro3EMData.GetLowestValue();
    float generatedPower = _PlugSData.GetActValue();

    if (IsLimitReached(generatedPower)) {
        float limit = FLT_MIN;
        float border = 5;
        if (gridPower > config.Shelly.TargetValue + border) {
            // increase: iterate to limit
            limit = _actLimit + abs(gridPower - config.Shelly.TargetValue) * 0.33f;

            // too little is generated, round to the next full hundred, avoid unnecessary limit set
            // if (limit > generatedPower + 50) {
            //    limit = (int)((generatedPower / 100.0) + 2) * 100;
            //}
        } else if (gridPower < -100) {
            // decrease: set new limit
            limit = config.Shelly.MinPower - config.Shelly.TargetValue;
            _Pro3EMData.SetLastValue(config.Shelly.TargetValue);

        } else if (gridPower < config.Shelly.TargetValue - border) {
            // decrease: set new limit
            // limit = _actLimit - abs(gridPower - config.Shelly.TargetValue) * 0.7f;
            limit = generatedPower - abs(gridPower - config.Shelly.TargetValue) * 0.7f;

            // eventuell nicht aktuelles Limit nehmen, sondern PlugS

            // decrease: set new lower limit
            // limit = gridPower + generatedPower - config.Shelly.TargetValue;

            _Pro3EMData.SetLastValue(config.Shelly.TargetValue);
            MessageOutput.printf("decrease %.1f +- %.1f, %.1f\r\n", gridPower, generatedPower - config.Shelly.TargetValue, limit);
        }

        if (limit != FLT_MIN) {
            if (SendLimit(limit, generatedPower)) {
                SendMqtt(limit, gridPower, generatedPower);
            }
        }
    }

    // small amount is produced
    if (_PlugSData.GetHighestValue() < 150) {
        return 5000;
    }

    if (_Pro3EMData.GetHighestValue() < 800 && 2 * _PlugSData.GetHighestValue() < _Pro3EMData.GetLowestValue()) {
        return 2500;
    }

    // maybe stove with alternating power, or other devices with high power consumption which can switch off any time
    return 500;
}

bool ShellyClientClass::SendLimit(float limit, float generatedPower)
{
    auto inv = Hoymiles.getInverterByPos(0);
    if (inv == nullptr) {
        return false;
    }

    float correctPanelCnt = 1.0; //0.75 => 3 of 4 pannels installed
    const CONFIG_T& config = Configuration.get();

    if (limit < config.Shelly.MinPower - config.Shelly.TargetValue) {
        limit = config.Shelly.MinPower - config.Shelly.TargetValue;
    }

    if (limit > config.Shelly.MaxPower) {
        limit = config.Shelly.MaxPower;
    }

    if (_actLimit == limit || abs(_actLimit - limit) < 10) {
        return false;
    }

    float lastCommand = millis() - _lastCommandTrigger;
    if (lastCommand < 3500) {
        return true;
    }

    _lastCommandTrigger = millis();

    _actLimit = limit;
    MessageOutput.printf("### SendLimit %.1f\r\n", limit);

    if (correctPanelCnt != 1) {
        limit /= correctPanelCnt;
        if (limit > config.Shelly.MaxPower) {
            limit = config.Shelly.MaxPower;
        }
    }
    inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent);

    return true;
}

uint32_t ShellyClientClass::getLastUpdate()
{
    uint32_t pro = _Pro3EMData.GetUpdateTime();
    uint32_t plugs = _PlugSData.GetUpdateTime();

    return pro > plugs ? pro : plugs;
}

bool ShellyClientClass::IsLimitReached(float generatedPower)
{
    if ((millis() - _lastCommandTrigger) < 2000) {
        float diff = abs(generatedPower - _actLimit);

        // Configuration.get().Shelly.TargetValue;
        // float l =
        // if (diff > _actLimit * 0.1) {
        //     return false;
        //}
    }
    return true;
}

void ShellyClientClass::SendMqtt(float limit, float pro3em_power, float plugs_power)
{
    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        const String subtopic = inv->serialString();

        MqttSettings.publish(subtopic + "/shelly/limit", String(limit));
        MqttSettings.publish(subtopic + "/shelly/pro3em_power", String(pro3em_power));
        MqttSettings.publish(subtopic + "/shelly/pro3em_time", String(_Pro3EMData.GetCircularBufferTime() / 1000));
        MqttSettings.publish(subtopic + "/shelly/plugs_power", String(plugs_power));
        MqttSettings.publish(subtopic + "/shelly/plugs_time", String(_PlugSData.GetCircularBufferTime() / 1000));
    }
}

String ShellyClientClass::getDebug()
{
    String info = String(_Pro3EMData.GetCircularBufferTime() / 1000) + ", " + String(_PlugSData.GetCircularBufferTime() / 1000);
    return info;
}
