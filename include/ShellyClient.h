// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>
#include <WebSocketsClient.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

////////////////////////

class WebSocketData {
public:
    WebSocketData()
        : Host("")
        , Client(nullptr)
        , LastMeasurement(0)
        , LastTime(0)
        , IsPro3EM(false)
    {
    }

    std::string Host;
    WebSocketsClient* Client;
    double LastMeasurement;
    unsigned long LastTime;
    bool Connected;
    bool IsPro3EM;
};

class ShellyClientData {
public:
    void Update(float pro3EMValue, float plugsValue, bool pro3EMValid, bool plugsValid);
    float GetValuePro3EM();
    float GetValuePlugS();
    bool GetValidPro3EM();
    bool GetValidPlugS();

private:
    std::mutex _mutex;

    float _Pro3EMValue;
    float _PlugsValue;
    bool _Pro3EMValid;
    bool _PlugsValid;
};

class ShellyClientClass {
public:
    ShellyClientClass() { }
    void init(Scheduler& scheduler);
    void loop();
    ShellyClientData& getData() { return _ShellyData; }

private:
    void HandleWebsocket(WebSocketData& data, const char* hostname, int poll_intervall, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent);
    static void EventsPro3EM(WStype_t type, uint8_t* payload, size_t length);
    static void EventsPlugS(WStype_t type, uint8_t* payload, size_t length);
    void Events(WebSocketData& data, WStype_t type, uint8_t* payload, size_t length);

    bool JSONPro3EM(WebSocketData& data, DynamicJsonDocument& root, const char* name1, const char* name2, const char* name3);

private:
    Task _loopTask;
    WebSocketData _Pro3EM;
    WebSocketData _PlugS;
    ShellyClientData _ShellyData;
};

extern ShellyClientClass ShellyClient;
