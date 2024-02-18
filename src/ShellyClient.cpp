// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClient.h"
#include "MessageOutput.h"
#include <Arduino.h>

ShellyClientClass ShellyClient;

void ShellyClientClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&ShellyClientClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
    _Pro3EM.IsPro3EM = true;
    _PlugS.IsPro3EM = false;
}

void ShellyClientClass::loop()
{
    while (WiFi.status() != WL_CONNECTED) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    HandleWebsocket(_Pro3EM, config.Shelly.Hostname_Pro3EM, config.Shelly.PollInterval, ShellyClientClass::EventsPro3EM);
    HandleWebsocket(_PlugS, config.Shelly.Hostname_PlugS, config.Shelly.PollInterval, ShellyClientClass::EventsPlugS);

    unsigned long t = millis();
    unsigned long max = config.Shelly.PollInterval * 1000;
    _ShellyData.Update(_Pro3EM.LastMeasurement, _PlugS.LastMeasurement, (t - _Pro3EM.LastTime) < max, (t - _PlugS.LastTime) < max);
}

void ShellyClientClass::HandleWebsocket(WebSocketData& data, const char* hostname, int poll_intervall, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent)
{
    bool bDelete = data.Host.compare(hostname) != 0; // hostname changed in configuration
    bDelete |= strlen(hostname) == 0; // IP deleted in configuration

    bool bNew = bDelete && strlen(hostname) > 0; // deleted and new hostname valid

    if (bDelete && data.Client != nullptr) {
        MessageOutput.printf("Delete WebSocketClient. %s\r\n", data.Host.c_str());
        delete data.Client;
        data.Client = nullptr;
        data.Host = "";
    }

    if (bNew) {
        MessageOutput.printf("Add WebSocketClient %s\r\n", hostname);
        data.Client = new WebSocketsClient();
        data.Client->begin(hostname, 80, "/rpc");
        data.Client->onEvent(cbEvent);
        data.Client->setReconnectInterval(2000);
        data.Host = hostname;
        data.LastTime = millis();
    }

    if (data.Connected && (millis() - data.LastTime > poll_intervall * 1000)) {
        data.LastTime = millis();
        MessageOutput.printf("SEND#########\r\n");
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
                    MessageOutput.printf("Pro3EM %f \r\n", data.LastMeasurement);
                }
            } else {
                if (JSONPro3EM(data, root, "result", "switch:0", "apower")) {
                    MessageOutput.printf("PlugS %f \r\n", data.LastMeasurement);
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

// ////////

void ShellyClientData::Update(float pro3EMValue, float plugsValue, bool pro3EMValid, bool plugsValid)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _Pro3EMValue = pro3EMValue;
    _PlugsValue = plugsValue;
    _Pro3EMValid = pro3EMValid;
    _PlugsValid = plugsValid;
}

float ShellyClientData::GetValuePro3EM()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _Pro3EMValue;
}

float ShellyClientData::GetValuePlugS()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _PlugsValue;
}

bool ShellyClientData::GetValidPro3EM()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _Pro3EMValid;
}

bool ShellyClientData::GetValidPlugS()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _PlugsValid;
}
