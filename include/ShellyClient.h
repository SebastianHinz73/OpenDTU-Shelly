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
        , LastValue(0)
        , LastTime(0)
        , UpdatedTime(0)
        , ShellyType(RamDataType_t::PlugS)
    {
    }

    std::string Host;
    WebSocketsClient* Client;
    double LastValue;
    unsigned long LastTime;
    unsigned long UpdatedTime;
    bool Connected;
    RamDataType_t ShellyType;
    uint32_t MaxInterval;
};

class ShellyClientClass {
public:
    ShellyClientClass();
    void init(Scheduler& scheduler);
    void loop();
    ShellyClientData& getShellyData() { return _shellyClientData; }

private:
    void HandleWebsocket(WebSocketData& data, const char* hostname, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent);
    static void EventsPro3EM(WStype_t type, uint8_t* payload, size_t length);
    static void EventsPlugS(WStype_t type, uint8_t* payload, size_t length);
    void Events(WebSocketData& data, WStype_t type, uint8_t* payload, size_t length);

private:
    std::mutex _mutex;
    Task _loopTask;
    WebSocketData _Pro3EM;
    WebSocketData _PlugS;
    ShellyClientData _shellyClientData;
};

extern ShellyClientClass ShellyClient;
