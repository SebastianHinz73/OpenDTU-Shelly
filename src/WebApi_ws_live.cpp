// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "ShellyClient.h"
#include "SunPosition.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>

#ifndef PIN_MAPPING_REQUIRED
#define PIN_MAPPING_REQUIRED 0
#endif

WebApiWsLiveClass::WebApiWsLiveClass()
    : _ws("/livedata")
    , _lastPublishShelly(0)
    , _wsCleanupTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsLiveClass::wsCleanupTaskCb, this))
    , _sendDataTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsLiveClass::sendDataTaskCb, this))
{
}

void WebApiWsLiveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    server.on("/api/livedata/status", HTTP_GET, std::bind(&WebApiWsLiveClass::onLivedataStatus, this, _1));
    server.on("/api/livedata/graph", HTTP_GET, std::bind(&WebApiWsLiveClass::onGraphUpdate, this, _1));

    server.addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));

    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.enable();

    scheduler.addTask(_sendDataTask);
    _sendDataTask.enable();
    _simpleDigestAuth.setUsername(AUTH_USERNAME);
    _simpleDigestAuth.setRealm("live websocket");

    reload();
}

void WebApiWsLiveClass::reload()
{
    _ws.removeMiddleware(&_simpleDigestAuth);

    auto const& config = Configuration.get();

    if (config.Security.AllowReadonly) {
        return;
    }

    _ws.enable(false);
    _simpleDigestAuth.setPassword(config.Security.Password);
    _ws.addMiddleware(&_simpleDigestAuth);
    _ws.closeAll();
    _ws.enable(true);
}

void WebApiWsLiveClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

void WebApiWsLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    auto generateJsonResponse = [&](std::shared_ptr<InverterAbstract> inv) {
        try {
            std::lock_guard<std::mutex> lock(_mutex);
            JsonDocument root;
            JsonVariant var = root;
            generateCommonJsonResponse(var);

            if (inv != nullptr) {
                auto invArray = var["inverters"].to<JsonArray>();
                auto invObject = invArray.add<JsonObject>();

                generateInverterCommonJsonResponse(invObject, inv);
                generateInverterChannelJsonResponse(invObject, inv);
            }

            if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
                return;
            }

            String buffer;
            serializeJson(root, buffer);

            _ws.textAll(buffer);

        } catch (const std::bad_alloc& bad_alloc) {
            MessageOutput.printf("Call to /api/livedata/status temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        } catch (const std::exception& exc) {
            MessageOutput.printf("Unknown exception in /api/livedata/status. Reason: \"%s\".\r\n", exc.what());
        }
    };

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        const uint32_t lastUpdateInternal = inv->Statistics()->getLastUpdateFromInternal();
        if (!((lastUpdateInternal > 0 && lastUpdateInternal > _lastPublishStats[i]) || (millis() - _lastPublishStats[i] > (10 * 1000)))) {
            continue;
        }

        _lastPublishStats[i] = millis();

        generateJsonResponse(inv);
    }

    if (Hoymiles.getNumInverters() == 0) {

        if (millis() > _lastPublishShelly + 3000) {
            generateJsonResponse(nullptr);
        }
    }
}

void WebApiWsLiveClass::generateCommonJsonResponse(JsonVariant& root)
{
    auto totalObj = root["total"].to<JsonObject>();
    addTotalField(totalObj, "Power", Datastore.getTotalAcPowerEnabled(), "W", Datastore.getTotalAcPowerDigits());
    addTotalField(totalObj, "YieldDay", Datastore.getTotalAcYieldDayEnabled(), "Wh", Datastore.getTotalAcYieldDayDigits());
    addTotalField(totalObj, "YieldTotal", Datastore.getTotalAcYieldTotalEnabled(), "kWh", Datastore.getTotalAcYieldTotalDigits());

    JsonObject hintObj = root["hints"].to<JsonObject>();
    struct tm timeinfo;
    hintObj["time_sync"] = !getLocalTime(&timeinfo, 5);
    hintObj["radio_problem"] = (Hoymiles.getRadioNrf()->isInitialized() && (!Hoymiles.getRadioNrf()->isConnected() || !Hoymiles.getRadioNrf()->isPVariant())) || (Hoymiles.getRadioCmt()->isInitialized() && (!Hoymiles.getRadioCmt()->isConnected()));
    hintObj["default_password"] = strcmp(Configuration.get().Security.Password, ACCESS_POINT_PASSWORD) == 0;

    hintObj["pin_mapping_issue"] = PIN_MAPPING_REQUIRED && !PinMapping.isMappingSelected();
}

void WebApiWsLiveClass::generateShellyCardJsonResponse(JsonVariant& root, ShellyViewOptions viewOptions)
{
    JsonObject shellyCards = root["cards"].to<JsonObject>();
    ShellyClientData& shellyData = ShellyClient.getShellyData();

    float gridPower = shellyData.GetFactoredValue(RamDataType_t::Pro3EM, 5000);
    float generatedPower = shellyData.GetFactoredValue(RamDataType_t::PlugS, 5000);

    shellyCards["pro3em_value"] = shellyData.GetActValue(RamDataType_t::Pro3EM);
    shellyCards["plugs_value"] = shellyData.GetActValue(RamDataType_t::PlugS);
    shellyCards["limit_value"] = shellyData.GetActValue(RamDataType_t::Limit);

    if (viewOptions >= ShellyViewOptions::CompleteInfo) {
        shellyCards["pro3em_debug"] = String(gridPower);
        shellyCards["plugs_debug"] = String(generatedPower);
        shellyCards["limit_debug"] = String(generatedPower);
    }

    addTotalField(shellyCards, "Power", Datastore.getTotalAcPowerEnabled(), "W", Datastore.getTotalAcPowerDigits());
}

void WebApiWsLiveClass::generateInverterCommonJsonResponse(JsonObject& root, std::shared_ptr<InverterAbstract> inv)
{
    const INVERTER_CONFIG_T* inv_cfg = Configuration.getInverterConfig(inv->serial());
    if (inv_cfg == nullptr) {
        return;
    }

    root["serial"] = inv->serialString();
    root["name"] = inv->name();
    root["order"] = inv_cfg->Order;
    root["data_age"] = (millis() - inv->Statistics()->getLastUpdate()) / 1000;
    root["poll_enabled"] = inv->getEnablePolling();
    root["reachable"] = inv->isReachable();
    root["producing"] = inv->isProducing();
    root["limit_relative"] = inv->SystemConfigPara()->getLimitPercent();
    if (inv->DevInfo()->getMaxPower() > 0) {
        root["limit_absolute"] = inv->SystemConfigPara()->getLimitPercent() * inv->DevInfo()->getMaxPower() / 100.0;
    } else {
        root["limit_absolute"] = -1;
    }
    root["radio_stats"]["tx_request"] = inv->RadioStats.TxRequestData;
    root["radio_stats"]["tx_re_request"] = inv->RadioStats.TxReRequestFragment;
    root["radio_stats"]["rx_success"] = inv->RadioStats.RxSuccess;
    root["radio_stats"]["rx_fail_nothing"] = inv->RadioStats.RxFailNoAnswer;
    root["radio_stats"]["rx_fail_partial"] = inv->RadioStats.RxFailPartialAnswer;
    root["radio_stats"]["rx_fail_corrupt"] = inv->RadioStats.RxFailCorruptData;
    root["radio_stats"]["rssi"] = inv->getLastRssi();
}

void WebApiWsLiveClass::generateInverterChannelJsonResponse(JsonObject& root, std::shared_ptr<InverterAbstract> inv)
{
    const INVERTER_CONFIG_T* inv_cfg = Configuration.getInverterConfig(inv->serial());
    if (inv_cfg == nullptr) {
        return;
    }

    // Loop all channels
    for (auto& t : inv->Statistics()->getChannelTypes()) {
        auto chanTypeObj = root[inv->Statistics()->getChannelTypeName(t)].to<JsonObject>();
        for (auto& c : inv->Statistics()->getChannelsByType(t)) {
            if (t == TYPE_DC) {
                chanTypeObj[String(static_cast<uint8_t>(c))]["name"]["u"] = inv_cfg->channel[c].Name;
            }
            addField(chanTypeObj, inv, t, c, FLD_PAC);
            addField(chanTypeObj, inv, t, c, FLD_UAC);
            addField(chanTypeObj, inv, t, c, FLD_IAC);
            if (t == TYPE_INV) {
                addField(chanTypeObj, inv, t, c, FLD_PDC, "Power DC");
            } else {
                addField(chanTypeObj, inv, t, c, FLD_PDC);
            }
            addField(chanTypeObj, inv, t, c, FLD_UDC);
            addField(chanTypeObj, inv, t, c, FLD_IDC);
            addField(chanTypeObj, inv, t, c, FLD_YD);
            addField(chanTypeObj, inv, t, c, FLD_YT);
            addField(chanTypeObj, inv, t, c, FLD_F);
            addField(chanTypeObj, inv, t, c, FLD_T);
            addField(chanTypeObj, inv, t, c, FLD_PF);
            addField(chanTypeObj, inv, t, c, FLD_Q);
            addField(chanTypeObj, inv, t, c, FLD_EFF);
            if (t == TYPE_DC && inv->Statistics()->getStringMaxPower(c) > 0) {
                addField(chanTypeObj, inv, t, c, FLD_IRR);
                chanTypeObj[String(c)][inv->Statistics()->getChannelFieldName(t, c, FLD_IRR)]["max"] = inv->Statistics()->getStringMaxPower(c);
            }
        }
    }

    if (inv->Statistics()->hasChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG)) {
        root["events"] = inv->EventLog()->getEntryCount();
    } else {
        root["events"] = -1;
    }
}

void WebApiWsLiveClass::addField(JsonObject& root, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId, String topic)
{
    if (inv->Statistics()->hasChannelFieldValue(type, channel, fieldId)) {
        String chanName;
        if (topic == "") {
            chanName = inv->Statistics()->getChannelFieldName(type, channel, fieldId);
        } else {
            chanName = topic;
        }
        String chanNum;
        chanNum = channel;
        root[chanNum][chanName]["v"] = inv->Statistics()->getChannelFieldValue(type, channel, fieldId);
        root[chanNum][chanName]["u"] = inv->Statistics()->getChannelFieldUnit(type, channel, fieldId);
        root[chanNum][chanName]["d"] = inv->Statistics()->getChannelFieldDigits(type, channel, fieldId);
    }
}

void WebApiWsLiveClass::addTotalField(JsonObject& root, const String& name, const float value, const String& unit, const uint8_t digits)
{
    root[name]["v"] = value;
    root[name]["u"] = unit;
    root[name]["d"] = digits;
}

void WebApiWsLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] connect\r\n", server->url(), client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] disconnect\r\n", server->url(), client->id());
    } else if (type == WS_EVT_DATA) {
        MessageOutput.printf("Websocket Type: [%d], len = %d\r\n", type, len);

        JsonDocument root;
        const DeserializationError error = deserializeJson(root, data);

        if (error) {
            MessageOutput.printf("error %s\r\n", data);
            return;
        }
        if (root["pro3em_value"].is<double_t>()) {
            MessageOutput.printf("found pro3em_value\r\n");

        } else {
            MessageOutput.printf("not found pro3em_value\r\n");
        }
        MessageOutput.printf("data %s\r\n", data);
    }
}

void WebApiWsLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutex);
        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();
        auto invArray = root["inverters"].to<JsonArray>();
        auto serial = WebApi.parseSerialFromRequest(request);

        if (serial > 0) {
            auto inv = Hoymiles.getInverterBySerial(serial);
            if (inv != nullptr) {
                JsonObject invObject = invArray.add<JsonObject>();
                generateInverterCommonJsonResponse(invObject, inv);
                generateInverterChannelJsonResponse(invObject, inv);
            }
        } else {
            // Loop all inverters
            for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
                auto inv = Hoymiles.getInverterByPos(i);
                if (inv == nullptr) {
                    continue;
                }

                JsonObject invObject = invArray.add<JsonObject>();
                generateInverterCommonJsonResponse(invObject, inv);
            }
        }

        generateCommonJsonResponse(root);

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/status temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/status. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

void WebApiWsLiveClass::generateDiagramJsonResponse(JsonVariant& root, String name, const RamDataType_t* types, int size)
{
    auto singleGraphArray = root[name].to<JsonArray>();

    for (int i = 0; i < size; i++) {
        auto singleGraphObject = singleGraphArray.add<JsonObject>();

        const char* name = "";
        const char* label = "";
        const char* color = "";
        switch (types[i]) {
        case RamDataType_t::Pro3EM:
            name = "data_pro3em";
            label = "Pro3em";
            color = "#ff0000";
            break;
        case RamDataType_t::Pro3EM_Min:
            name = "data_pro3em_min";
            label = "Min";
            color = "#c8c8c8";
            break;
        case RamDataType_t::Pro3EM_Max:
            name = "data_pro3em_max";
            label = "Max";
            color = "#646464";
            break;

        case RamDataType_t::PlugS:
            name = "data_plugs";
            label = "PlugS";
            color = "#0000FF";
            break;
        case RamDataType_t::PlugS_Min:
            name = "data_plugs_min";
            label = "Min";
            color = "#c8c8c8";
            break;
        case RamDataType_t::PlugS_Max:
            name = "data_plugs_max";
            label = "Max";
            color = "#646464";
            break;

        case RamDataType_t::CalulatedLimit:
            name = "data_calculated_limit";
            label = "Calc Limit";
            color = "#00FF00";
            break;
        case RamDataType_t::Limit:
            name = "data_limit";
            label = "Limit";
            color = "#00aa00";
            break;

        default:
            break;
        };

        singleGraphObject["data_name"] = name;
        singleGraphObject["label"] = label;
        singleGraphObject["color"] = color;
    }
}

void WebApiWsLiveClass::onGraphUpdate(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutex);

        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();

        const CONFIG_T& config = Configuration.get();
        ShellyClientData& shellyData = ShellyClient.getShellyData();

        ShellyViewOptions viewOptions = static_cast<ShellyViewOptions>(config.Shelly.ViewOption);
        if (viewOptions >= ShellyViewOptions::SimpleInfo) {
            root["view_option"] = config.Shelly.ViewOption;

            generateShellyCardJsonResponse(root, viewOptions);

            long long timestamp = 0;
            uint32_t interval = 2 * TASK_SECOND;

            if (request->hasParam("timestamp")) {
                String s = request->getParam("timestamp")->value();

                timestamp = strtoll(s.c_str(), NULL, 10);
                if (timestamp == 0) {
                    interval = 60 * TASK_SECOND;
                }
            }

            root["timestamp"] = timestamp + interval;
            root["interval"] = interval;

            if (viewOptions >= ShellyViewOptions::DiagramInfo) {
                String data;

                root["data_pro3em"] = shellyData.GetLastData(RamDataType_t::Pro3EM, interval, data);
                root["data_pro3em_min"] = shellyData.GetLastData(RamDataType_t::Pro3EM_Min, interval, data);
                root["data_pro3em_max"] = shellyData.GetLastData(RamDataType_t::Pro3EM_Max, interval, data);

                root["data_plugs"] = shellyData.GetLastData(RamDataType_t::PlugS, interval, data);
                root["data_plugs_min"] = shellyData.GetLastData(RamDataType_t::PlugS_Min, interval, data);
                root["data_plugs_max"] = shellyData.GetLastData(RamDataType_t::PlugS_Max, interval, data);

                root["data_calculated_limit"] = shellyData.GetLastData(RamDataType_t::CalulatedLimit, interval, data);
                root["data_limit"] = shellyData.GetLastData(RamDataType_t::Limit, interval, data);

                const RamDataType_t a[] = { RamDataType_t::Pro3EM, RamDataType_t::Pro3EM_Min, RamDataType_t::Pro3EM_Max };
                generateDiagramJsonResponse(root, "diagram_pro3em", a, 3);

                const RamDataType_t b[] = { RamDataType_t::PlugS, RamDataType_t::PlugS_Min, RamDataType_t::PlugS_Max };
                generateDiagramJsonResponse(root, "diagram_plugs", b, 3);

                const RamDataType_t c[] = { RamDataType_t::CalulatedLimit, RamDataType_t::Limit };
                generateDiagramJsonResponse(root, "diagram_limit", c, 2);

                const RamDataType_t d[] = { RamDataType_t::Pro3EM, RamDataType_t::PlugS, RamDataType_t::Limit };
                generateDiagramJsonResponse(root, "diagram_all", d, 3);
            }
        }

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/graph temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/graph. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}
