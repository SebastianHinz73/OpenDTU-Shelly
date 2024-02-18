// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_shelly.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

WebApiShellyClass::WebApiShellyClass()
    : _applyDataTask(TASK_IMMEDIATE, TASK_ONCE, std::bind(&WebApiShellyClass::applyDataTaskCb, this))
{
}

void WebApiShellyClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/shelly/config", HTTP_GET, std::bind(&WebApiShellyClass::onShellyAdminGet, this, _1));
    server.on("/api/shelly/config", HTTP_POST, std::bind(&WebApiShellyClass::onShellyAdminPost, this, _1));

    scheduler.addTask(_applyDataTask);
}

void WebApiShellyClass::applyDataTaskCb()
{
    // Execute stuff in main thread to avoid busy SPI bus
    /* CONFIG_T& config = Configuration.get();
     Hoymiles.getRadioNrf()->setPALevel((rf24_pa_dbm_e)config.Dtu.Nrf.PaLevel);
     Hoymiles.getRadioCmt()->setPALevel(config.Dtu.Cmt.PaLevel);
     Hoymiles.getRadioNrf()->setDtuSerial(config.Dtu.Serial);
     Hoymiles.getRadioCmt()->setDtuSerial(config.Dtu.Serial);
     Hoymiles.getRadioCmt()->setCountryMode(static_cast<CountryModeId_t>(config.Dtu.Cmt.CountryMode));
     Hoymiles.getRadioCmt()->setInverterTargetFrequency(config.Dtu.Cmt.Frequency);
     Hoymiles.setPollInterval(config.Dtu.PollInterval);
 */
}

void WebApiShellyClass::onShellyAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["shelly_hostname_pro3em"] = config.Shelly.Hostname_Pro3EM;
    root["shelly_hostname_plugs"] = config.Shelly.Hostname_PlugS;
    root["pollinterval"] = config.Shelly.PollInterval;
    root["max_power"] = config.Shelly.MaxPower;
    root["limit_power"] = config.Shelly.LimitPower;

    response->setLength();
    request->send(response);
}

void WebApiShellyClass::onShellyAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    const String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    const DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("shelly_hostname_pro3em")
            && root.containsKey("shelly_hostname_plugs")
            && root.containsKey("pollinterval")
            && root.containsKey("max_power")
            && root.containsKey("limit_power"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["shelly_hostname_pro3em"].as<String>().length() > SHELLY_MAX_HOSTNAME_STRLEN) {
        retMsg["message"] = "Shelly Pro 3EM  must maximal " STR(SHELLY_MAX_HOSTNAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::MqttHostnameLength;
        retMsg["param"]["max"] = SHELLY_MAX_HOSTNAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }
    if (root["shelly_hostname_plugs"].as<String>().length() > SHELLY_MAX_HOSTNAME_STRLEN) {
        retMsg["message"] = "Shelly Plug S must maximal " STR(SHELLY_MAX_HOSTNAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::MqttHostnameLength;
        retMsg["param"]["max"] = SHELLY_MAX_HOSTNAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }
    if (root["pollinterval"].as<uint32_t>() == 0) {
        retMsg["message"] = "Poll interval must be greater zero!";
        retMsg["code"] = WebApiError::DtuPollZero;
        response->setLength();
        request->send(response);
        return;
    }
    if (root["max_power"].as<uint32_t>() <= 0 || root["max_power"].as<uint32_t>() >= 3000) {
        retMsg["message"] = "Max power must be greater zero and less or equal than 3000!";
        retMsg["code"] = WebApiError::DtuPollZero;
        response->setLength();
        request->send(response);
        return;
    }
    if (root["limit_power"].as<uint32_t>() <= 0 || root["limit_power"].as<uint32_t>() >= 3000) {
        retMsg["message"] = "Max power must be greater zero and less or equal than 3000!";
        retMsg["code"] = WebApiError::DtuPollZero;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    strlcpy(config.Shelly.Hostname_Pro3EM, root["shelly_hostname_pro3em"].as<String>().c_str(), sizeof(config.Shelly.Hostname_Pro3EM));
    strlcpy(config.Shelly.Hostname_PlugS, root["shelly_hostname_plugs"].as<String>().c_str(), sizeof(config.Shelly.Hostname_PlugS));
    config.Shelly.PollInterval = root["pollinterval"].as<uint32_t>();
    config.Shelly.MaxPower = root["max_power"].as<uint32_t>();
    config.Shelly.LimitPower = root["limit_power"].as<uint32_t>();

    WebApi.writeConfig(retMsg);

    response->setLength();
    request->send(response);

    _applyDataTask.enable();
}
