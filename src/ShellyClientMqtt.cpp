// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClientMqtt.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include <Hoymiles.h>
#include <cfloat>
#include <esp32-hal.h>

ShellyClientMqttClass ShellyClientMqtt;

std::map<ShellyClientMqttType_t, ShellyClientMqttClass::data> ShellyClientMqttClass::_data_map = {
    { ShellyClientMqttType_t::Limit, { "/shelly/limit", 0, 0 } },
    { ShellyClientMqttType_t::Pro3EMPower, { "/shelly/pro3em_power", 0, 0 } },
    { ShellyClientMqttType_t::Pro3EMTime, { "/shelly/pro3em_time", 0, 0 } },
    { ShellyClientMqttType_t::PlugSPower, { "/shelly/plugs_power", 0, 0 } },
    { ShellyClientMqttType_t::PlugSTime, { "/shelly/plugs_time", 0, 0 } },
};

ShellyClientMqttClass::ShellyClientMqttClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&ShellyClientMqttClass::loop, this))
{
}

void ShellyClientMqttClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

/*
void ShellyClientMqttClass::Update(ShellyClientMqttType_t id, float value)
{
    std::map<ShellyClientMqttType_t, ShellyClientMqttClass::data>::iterator it = _data_map.find(id);
    if (it != _data_map.end()) {
        float rValue = static_cast<int>(value * 10.0) / 10.0f;
        if (rValue != it->second.lastSendValue) {
            it->second.lastUpdateTime = millis();
            it->second.lastSendValue = rValue;
            MqttSettings.publish(_Topic + it->second.topic, String(it->second.lastSendValue));
        }
    }
}
*/

void ShellyClientMqttClass::loop()
{
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

#if 0
    // TODO
    
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

    std::map<ShellyClientMqttType_t, ShellyClientMqttClass::data>::iterator it;
    for (it = _data_map.begin(); it != _data_map.end(); it++) {
        if (it->second.lastUpdateTime > 0 && it->second.lastUpdateTime + 5000 <= now) {
            MqttSettings.publish(_Topic + it->second.topic, String(it->second.lastSendValue));
            it->second.lastUpdateTime = now;
        }
    }
#endif
}
