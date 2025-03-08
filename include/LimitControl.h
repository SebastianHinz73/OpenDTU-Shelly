// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "ShellyClientData.h"

// #include <ArduinoJson.h>
// #include <TaskSchedulerDeclarations.h>
// #include <WebSocketsClient.h>
// #include <cstdint>
// #include <memory>
// #include <mutex>
// #include <string>
// #include <vector>

////////////////////////

#if 1
enum SendLimitResult_t {
    NoInverter,
    Similar,
    CommandPending,
    SendOk,
};

class LimitControlClass {
public:
    LimitControlClass();
    void init(Scheduler& scheduler);
    void loop();

private:
    void CalculateLimit();
    SendLimitResult_t SendLimit(float limit, float generatedPower);

private:
    //  std::mutex _mutex;
    Task _loopTask;
    ShellyClientData& _shellyClientData;

    float _invLimitAbsolute;
    time_t _intervalPro3em;
    time_t _intervalPlugS;

    float _actLimit;
    unsigned long _lastLimitSend;
};

extern LimitControlClass LimitControl;
#endif