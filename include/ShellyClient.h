// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "ShellyClientData.h"
#include "ShellyClientMqtt.h"
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
        , ShellyType(ShellyClientType_t::PlugS)
    {
    }

    std::string Host;
    WebSocketsClient* Client;
    double LastMeasurement;
    unsigned long LastTime;
    bool Connected;
    ShellyClientType_t ShellyType;
    uint32_t MaxInterval;
};

enum SendLimitResult_t {
    NoInverter,
    Similar,
    CommandPending,
    SendOk,
};

class ShellyClientClass {
public:
    ShellyClientClass();
    void init(Scheduler& scheduler);
    void loop();
    ShellyClientData& getShellyData() { return _shellyClientData; }
    float getActLimit() { return _actLimit; }
    uint32_t getLastUpdate();
    String getDebug();
    uint32_t GetPollInterval(ShellyClientType_t type) { return type == ShellyClientType_t::PlugS ? _PlugS.MaxInterval : _Pro3EM.MaxInterval; }

private:
    void HandleWebsocket(WebSocketData& data, const char* hostname, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent);
    static void EventsPro3EM(WStype_t type, uint8_t* payload, size_t length);
    static void EventsPlugS(WStype_t type, uint8_t* payload, size_t length);
    void Events(WebSocketData& data, WStype_t type, uint8_t* payload, size_t length);

    void SetLimit();
    SendLimitResult_t SendLimit(float limit, float generatedPower);
    float IncreaseFactor(float diff);
    float DecreaseFactor(float diff);
    void Debug(const char* text);
    void Debug(float number);

private:
    std::mutex _mutex;
    Task _loopTask;
    WebSocketData _Pro3EM;
    WebSocketData _PlugS;
    ShellyClientData _shellyClientData;
    ShellyClientMqtt _shellyClientMqtt;
    unsigned long _lastCommandTrigger;
    float _actLimit;
    int _increaseCnt;
    String _Debug;
};

extern ShellyClientClass ShellyClient;
