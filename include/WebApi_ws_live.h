// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "RamBuffer.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>

class WebApiWsLiveClass {
private:
enum class ShellyViewOptions {
    NoInfo,
    SimpleInfo,
    DiagramInfo,
    CompleteInfo
};

public:
    WebApiWsLiveClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);
    void reload();

private:
    static void generateInverterCommonJsonResponse(JsonObject& root, std::shared_ptr<InverterAbstract> inv);
    static void generateInverterChannelJsonResponse(JsonObject& root, std::shared_ptr<InverterAbstract> inv);
    static void generateCommonJsonResponse(JsonVariant& root);
    static void generateShellyCardJsonResponse(JsonVariant& root, ShellyViewOptions viewOptions);

    static void addField(JsonObject& root, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId, String topic = "");
    static void addTotalField(JsonObject& root, const String& name, const float value, const String& unit, const uint8_t digits);
    static void generateDiagramJsonResponse(JsonVariant& root, String name, const RamDataType_t* types, int size);

    void onLivedataStatus(AsyncWebServerRequest* request);
    void onGraphUpdate(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebSocket _ws;
    AsyncAuthenticationMiddleware _simpleDigestAuth;

    uint32_t _lastPublishStats[INV_MAX_COUNT] = { 0 };
    uint32_t _lastPublishShelly;

    std::mutex _mutex;

    Task _wsCleanupTask;
    void wsCleanupTaskCb();

    Task _sendDataTask;
    void sendDataTaskCb();
};
