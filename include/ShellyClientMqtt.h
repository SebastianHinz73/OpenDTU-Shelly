// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>

#include <map>
#include <time.h>

enum ShellyClientMqttType_t {
    Limit,
    Pro3EMPower,
    Pro3EMTime,
    PlugSPower,
    PlugSTime,
};

class ShellyClientMqttClass {
public:
    ShellyClientMqttClass();
    void init(Scheduler& scheduler);

    // void Update(ShellyClientMqttType_t id, float value);
    void loop();

private:
    Task _loopTask;

    struct data {
        const char* topic;
        int lastUpdateTime;
        float lastSendValue;
    };
    static std::map<ShellyClientMqttType_t, data> _data_map;
    String _Topic;
};

extern ShellyClientMqttClass ShellyClientMqtt;
