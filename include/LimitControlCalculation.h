// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ILimitControlHoymiles.h"
#include "IShellyClientData.h"
#include "ITimeLapse.h"

typedef struct {
    uint32_t _consecutiveCnt;

} CalcLimitFunctionData_t;

#ifndef INV_MAX_CHAN_COUNT
#define INV_MAX_CHAN_COUNT 6
#endif

class LimitControlCalculation {
public:
    LimitControlCalculation();
    void loop(IShellyClientData& shellyClientData, ILimitControlHoymiles& hoymiles, ITimeLapse& timeLapse);

private:
    bool CalculateLimit(IShellyClientData& shellyClientData, float& limit);
    float CorrectChannelPower(float channelPower);
    float CheckBoundary(float limit);

    float Increase(CalcLimitFunctionData_t& context, float gridPower);
    float Decrease(CalcLimitFunctionData_t& context, float gridPower, float generatedPower);
    float Optimize(CalcLimitFunctionData_t& context);
    void HandleCnt(CalcLimitFunctionData_t& context);

private:
    float _invLimitAbsolute;
    time_t _intervalPro3em;
    time_t _intervalPlugS;

    float _actLimit;
    unsigned long _lastLimitSend;
    std::string _debugPro3em;
    std::string _debugPlugS;
    float _channelPower[INV_MAX_CHAN_COUNT];
    int _channelCnt;

    CalcLimitFunctionData_t _dataIncrease;
    CalcLimitFunctionData_t _dataDecrease;
    CalcLimitFunctionData_t _dataOptimize;
};
