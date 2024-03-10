// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClient.h"
#include "MessageOutput.h"
#include "SunPosition.h"
#include <Arduino.h>
#include <Hoymiles.h>

ShellyClientClass ShellyClient;

void ShellyClientClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&ShellyClientClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
    _Pro3EM.IsPro3EM = true;
    _PlugS.IsPro3EM = false;
    _actLimit = 0;
}

void ShellyClientClass::loop()
{
    while (WiFi.status() != WL_CONNECTED) {
        return;
    }
    if(!SunPosition.isDayPeriod()) {
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
        MessageOutput.printf("SEND %s#####\r\n", data.IsPro3EM ? "Pro" : "PlugS");
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
                    _Pro3EMData.Update(data.LastMeasurement, "Pro");
                }
            } else {
                if (JSONPro3EM(data, root, "result", "switch:0", "apower")) {
                    _PlugSData.Update(data.LastMeasurement, "PlugS");
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
    float limit = gridPower + generatedPower - config.Shelly.TargetValue;

    SendLimit(limit, generatedPower);

    if (_PlugSData.GetHighestValue() < 150) {
        return 5000; // small amount is produced
    }

    if (_Pro3EMData.GetHighestValue() < 800 && 2 * _PlugSData.GetHighestValue() < _Pro3EMData.GetLowestValue()) {
        return 2500;
    }
    return 500;
}

void ShellyClientClass::SendLimit(float limit, float generatedPower)
{
    auto inv = Hoymiles.getInverterByPos(0);
    if (inv == nullptr) {
        return;
    }

    float correctPanelCnt = 0.75; // 3 of 4 pannels installed
    const CONFIG_T& config = Configuration.get();

    // too little is generated, round to the next full hundred, avoid unnecessary limit set
    if (limit > generatedPower + 50) {
        limit = generatedPower + 50;
        limit = (int)((limit / 100.0) + 2) * 100;
    }

    if (limit > config.Shelly.MaxPower) {
        limit = config.Shelly.MaxPower;
    }

    // MessageOutput.printf("SendLimit ?? %.1f\r\n", limit);

    if (_actLimit == limit || abs(_actLimit - limit) < 10) {
        return;
    }
    MessageOutput.printf("SendLimit %.1f\r\n", limit);

    float lastCommand = millis() - _lastCommandTrigger;
    if (_actLimit < limit && lastCommand < 2500) {
        return;
    }
    if (limit < _actLimit && lastCommand < 1000) {
        return;
    }
    MessageOutput.printf("### SendLimit %.1f\r\n", limit);

    _lastCommandTrigger = millis();

    if (inv != nullptr) {
        _actLimit = limit;

        if (correctPanelCnt != 1) {
            limit /= correctPanelCnt;
            if (limit > 1600) {
                limit = 1600;
            }
        }
        inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent);
    }
}
