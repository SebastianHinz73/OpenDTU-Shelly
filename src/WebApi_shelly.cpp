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
{
}

void WebApiShellyClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/shelly/config", HTTP_GET, std::bind(&WebApiShellyClass::onShellyAdminGet, this, _1));
    server.on("/api/shelly/config", HTTP_POST, std::bind(&WebApiShellyClass::onShellyAdminPost, this, _1));
}

void WebApiShellyClass::onShellyAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["shelly_enable"] = config.Shelly.ShellyEnable;
    root["shelly_hostname_pro3em"] = config.Shelly.Hostname_Pro3EM;
    root["shelly_hostname_plugs"] = config.Shelly.Hostname_PlugS;
    root["limit_enable"] = config.Shelly.LimitEnable;
    root["max_power"] = config.Shelly.MaxPower;
    root["min_power"] = config.Shelly.MinPower;
    root["target_value"] = config.Shelly.TargetValue;
    root["feed_in_level"] = config.Shelly.FeedInLevel;
    root["view_option"] = config.Shelly.ViewOption;

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

    JsonDocument root;
    const DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root["shelly_enable"].is<bool>()
            && root["shelly_hostname_pro3em"].is<String>()
            && root["shelly_hostname_plugs"].is<String>()
            && root["limit_enable"].is<bool>()
            && root["max_power"].is<uint32_t>()
            && root["min_power"].is<uint32_t>()
            && root["target_value"].is<int32_t>()
            && root["view_option"].is<uint32_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["shelly_enable"].as<bool>()) {
        if (root["shelly_hostname_pro3em"].as<String>().length() > SHELLY_MAX_HOSTNAME_STRLEN) {
            retMsg["message"] = "Shelly Pro 3EM  must maximal " STR(SHELLY_MAX_HOSTNAME_STRLEN) " characters long!";
            retMsg["code"] = WebApiError::ShellyHostnameLength;
            retMsg["param"]["max"] = SHELLY_MAX_HOSTNAME_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }
        if (root["shelly_hostname_plugs"].as<String>().length() > SHELLY_MAX_HOSTNAME_STRLEN) {
            retMsg["message"] = "Shelly Plug S must maximal " STR(SHELLY_MAX_HOSTNAME_STRLEN) " characters long!";
            retMsg["code"] = WebApiError::ShellyHostnameLength;
            retMsg["param"]["max"] = SHELLY_MAX_HOSTNAME_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["limit_enable"].as<bool>()) {
            if (root["max_power"].as<uint32_t>() <= 0 || root["max_power"].as<uint32_t>() > 3000) {
                retMsg["message"] = "Max power must be greater zero and less than 3000!";
                retMsg["code"] = WebApiError::MaxPowerLimit;
                retMsg["param"]["min"] = 0;
                retMsg["param"]["max"] = 3000;
                response->setLength();
                request->send(response);
                return;
            }
            if (root["min_power"].as<int32_t>() < 0 || root["min_power"].as<uint32_t>() > 500) {
                retMsg["message"] = "Min power must be greater or equal zero and less than 500!";
                retMsg["code"] = WebApiError::MinPowerLimit;
                retMsg["param"]["min"] = 0;
                retMsg["param"]["max"] = 500;
                response->setLength();
                request->send(response);
                return;
            }
            if (root["target_value"].as<int32_t>() < -100 || root["target_value"].as<int32_t>() > 300) {
                retMsg["message"] = "The target value must be greater -100 and less than 100!";
                retMsg["code"] = WebApiError::TargetValueLimit;
                retMsg["param"]["min"] = -100;
                retMsg["param"]["max"] = 100;
                response->setLength();
                request->send(response);
                return;
            }
        }
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        config.Shelly.ShellyEnable = root["shelly_enable"].as<bool>();
        strlcpy(config.Shelly.Hostname_Pro3EM, root["shelly_hostname_pro3em"].as<String>().c_str(), sizeof(config.Shelly.Hostname_Pro3EM));
        strlcpy(config.Shelly.Hostname_PlugS, root["shelly_hostname_plugs"].as<String>().c_str(), sizeof(config.Shelly.Hostname_PlugS));
        config.Shelly.LimitEnable = root["limit_enable"].as<bool>();
        config.Shelly.MaxPower = root["max_power"].as<uint32_t>();
        config.Shelly.MinPower = root["min_power"].as<uint32_t>();
        config.Shelly.TargetValue = root["target_value"].as<int32_t>();
        config.Shelly.FeedInLevel = root["feed_in_level"].as<uint32_t>();
        config.Shelly.ViewOption = root["view_option"].as<uint32_t>();
    }
    WebApi.writeConfig(retMsg);

    response->setLength();
    request->send(response);
}
