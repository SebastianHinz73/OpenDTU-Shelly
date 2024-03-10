// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "ShellyClientData.h"
#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>
#include <WebSocketsClient.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

////////////////////////

#define SHELLY_DTU_VERSION "0.9"

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

class ShellyClientClass {
public:
    ShellyClientClass() { }
    void init(Scheduler& scheduler);
    void loop();
    ShellyClientData& getPro3EMData() { return _Pro3EMData; }
    ShellyClientData& getPlugSData() { return _PlugSData; }
    float getActLimit() { return _actLimit; }

private:
    void HandleWebsocket(WebSocketData& data, const char* hostname, int poll_intervall, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent);
    static void EventsPro3EM(WStype_t type, uint8_t* payload, size_t length);
    static void EventsPlugS(WStype_t type, uint8_t* payload, size_t length);
    void Events(WebSocketData& data, WStype_t type, uint8_t* payload, size_t length);

    bool JSONPro3EM(WebSocketData& data, DynamicJsonDocument& root, const char* name1, const char* name2, const char* name3);
    int SetLimit();
    void SendLimit(float limit, float generatedPower);

private:
    Task _loopTask;
    WebSocketData _Pro3EM;
    WebSocketData _PlugS;
    ShellyClientData _Pro3EMData;
    ShellyClientData _PlugSData;
    unsigned long _lastCommandTrigger;
    float _actLimit;
};

extern ShellyClientClass ShellyClient;
