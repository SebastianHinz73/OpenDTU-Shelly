// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <map>
#include <time.h>

enum ShellyClientMqttType_t {
    Limit,
    Pro3EMPower,
    Pro3EMTime,
    PlugSPower,
    PlugSTime,
};

class ShellyClientMqtt {
public:
    void Update(ShellyClientMqttType_t id, float value);
    void Loop();

private:
    struct data {
        const char* topic;
        int lastUpdateTime;
        float lastSendValue;
    };
    static std::map<ShellyClientMqttType_t, data> _data_map;
    String _Topic;
};
