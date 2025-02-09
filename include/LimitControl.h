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
    /*
    ShellyClientData& getShellyData() { return _shellyClientData; }
    float getActLimit() { return _actLimit; }
    uint32_t getLastUpdate();
    String getDebug();
    uint32_t GetPollInterval(ShellyClientType_t type) { return type == ShellyClientType_t::PlugS ? _PlugS.MaxInterval : _Pro3EM.MaxInterval; }
*/

private:
    void CalculateLimit();
    SendLimitResult_t SendLimit(float limit, float generatedPower);
    // float IncreaseFactor(float diff);
    // float DecreaseFactor(float diff);
    // void Debug(const char* text);
    // void Debug(float number);

private:
    //  std::mutex _mutex;
    Task _loopTask;
    ShellyClientData& _shellyClientData;

    float _invLimitAbsolute;
    time_t _intervalPro3em;
    time_t _intervalPlugS;

    float _actLimit;
    unsigned long _lastLimitSend;

    //  unsigned long _lastCommandTrigger;
    //  float _actLimit;
    //  int _increaseCnt;
    //  String _Debug;
};

extern LimitControlClass LimitControl;
#endif