// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClientMqtt.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include <Hoymiles.h>
#include <cfloat>
#include <esp32-hal.h>

std::map<ShellyClientMqttType_t, ShellyClientMqtt::data> ShellyClientMqtt::_data_map = {
    { ShellyClientMqttType_t::Limit, { "/shelly/limit", 0, 0 } },
    { ShellyClientMqttType_t::Pro3EMPower, { "/shelly/pro3em_power", 0, 0 } },
    { ShellyClientMqttType_t::Pro3EMTime, { "/shelly/pro3em_time", 0, 0 } },
    { ShellyClientMqttType_t::PlugSPower, { "/shelly/plugs_power", 0, 0 } },
    { ShellyClientMqttType_t::PlugSTime, { "/shelly/plugs_time", 0, 0 } },
};

void ShellyClientMqtt::Update(ShellyClientMqttType_t id, float value)
{
    std::map<ShellyClientMqttType_t, ShellyClientMqtt::data>::iterator it = _data_map.find(id);
    if (it != _data_map.end()) {
        float rValue = static_cast<int>(value * 10.0) / 10.0f;
        if (rValue != it->second.lastSendValue) {
            it->second.lastUpdateTime = millis();
            it->second.lastSendValue = rValue;
            MqttSettings.publish(_Topic + it->second.topic, String(it->second.lastSendValue));
        }
    }
}

void ShellyClientMqtt::Loop()
{
    // initialize _Topic
    if (_Topic.isEmpty()) {
        if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
            return;
        }
        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            _Topic = inv->serialString();
            break;
        }
    }
    unsigned long now = millis();

    std::map<ShellyClientMqttType_t, ShellyClientMqtt::data>::iterator it;
    for (it = _data_map.begin(); it != _data_map.end(); it++) {
        if (it->second.lastUpdateTime > 0 && it->second.lastUpdateTime + 5000 <= now) {
            MqttSettings.publish(_Topic + it->second.topic, String(it->second.lastSendValue));
            it->second.lastUpdateTime = now;
        }
    }
}
