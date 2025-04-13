// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "ShellyClientData.h"

enum SendLimitResult_t {
    NoInverter,
    Similar,
    CommandPending,
    SendOk,
};

typedef struct {
    uint32_t _consecutiveCnt;

} CalcLimitFunctionData_t;

class InverterAbstract;

class LimitControlClass {
public:
    LimitControlClass();
    void init(Scheduler& scheduler);
    void loop();

private:
    bool CalculateLimit(float& limit);
    void FetchChannelPower(std::shared_ptr<InverterAbstract> inv);
    float CorrectChannelPower(float channelPower);
    float CheckBoundary(float limit);

    float Increase(CalcLimitFunctionData_t& context, float gridPower);
    float Decrease(CalcLimitFunctionData_t& context, float gridPower, float generatedPower);
    float Optimize(CalcLimitFunctionData_t& context);
    void HandleCnt(CalcLimitFunctionData_t& context);

private:
    Task _loopTask;
    ShellyClientData& _shellyClientData;

    float _invLimitAbsolute;
    time_t _intervalPro3em;
    time_t _intervalPlugS;

    float _actLimit;
    unsigned long _lastLimitSend;
    String _debugPro3em;
    String _debugPlugS;
    float _channelPower[INV_MAX_CHAN_COUNT];
    int _channelCnt;
    CalcLimitFunctionData_t _dataIncrease;
    CalcLimitFunctionData_t _dataDecrease;
    CalcLimitFunctionData_t _dataOptimize;
};

extern LimitControlClass LimitControl;
