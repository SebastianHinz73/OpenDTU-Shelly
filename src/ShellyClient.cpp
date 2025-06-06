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

    _Pro3EM.ShellyType = RamDataType_t::Pro3EM;
    _Pro3EM.MaxInterval = 5000;
    _PlugS.ShellyType = RamDataType_t::PlugS;
    _PlugS.MaxInterval = 5000;
}

void ShellyClientClass::loop()
{
    while (WiFi.status() != WL_CONNECTED) {
        return;
    }
    // if (!SunPosition.isDayPeriod()) {
    //     return;
    // }

    const CONFIG_T& config = Configuration.get();

    HandleWebsocket(_Pro3EM, config.Shelly.Hostname_Pro3EM, ShellyClientClass::EventsPro3EM);
    HandleWebsocket(_PlugS, config.Shelly.Hostname_PlugS, ShellyClientClass::EventsPlugS);
}

void ShellyClientClass::HandleWebsocket(WebSocketData& data, const char* hostname, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent)
{
    const CONFIG_T& config = Configuration.get();
    unsigned long nowMillis = millis();

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
        data.LastTime = nowMillis;
        data.UpdatedTime = data.LastTime;
    }

    if (data.Connected) {
        if (nowMillis - data.LastTime > data.MaxInterval) {
            data.Client->sendTXT("{\"id\":2, \"src\":\"user_1\", \"method\":\"Shelly.GetStatus\"}");

            std::lock_guard<std::mutex> lock(_mutex);
            data.LastTime = nowMillis;
            data.UpdatedTime = data.LastTime;
        } else if (nowMillis - data.UpdatedTime > 1000) {
            _shellyClientData.Update(data.ShellyType, data.LastValue);
            data.UpdatedTime += 1000;
        }
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
    case WStype_TEXT: {
        auto ParseDouble = [&](const char* search, double& result) {
            // e.g. ...ull,"total_act_power":225.658,"total
            // e.g. ...ue, "apower":0.0, "volta
            char* key = strstr(reinterpret_cast<char*>(payload), search);
            if (key != nullptr) {
                key += strlen(search);
                result = atof(key);
                return true;
            }
            return false;
        };

        if (data.ShellyType == RamDataType_t::Pro3EM) {
            if (ParseDouble("\"total_act_power\":", data.LastValue)) {
                _shellyClientData.Update(data.ShellyType, data.LastValue);
            }
        } else {
            if (ParseDouble("\"apower\":", data.LastValue)) {
                _shellyClientData.Update(data.ShellyType, data.LastValue);
            }
        }
    } break;
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

    std::lock_guard<std::mutex> lock(_mutex);
    data.LastTime = millis();
    data.UpdatedTime = data.LastTime;
}
