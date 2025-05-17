// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IShellyWrapper.h"
#include "LimitControlCalculation.h"
#include "ShellyClientData.h"
#include <fstream>
#include <mutex>

typedef struct
{
    time_t time;
    float value;
} TestEntry_t;

class ShellyWrapperMock : public ShellyClientData, public LimitControlCalculation, public IShellyWrapper {
public:
    ShellyWrapperMock(float factor = 1.0);
    virtual ~ShellyWrapperMock();

    virtual bool isReachable() { return true; }
    virtual bool sendLimit(float limit);
    virtual int fetchChannelPower(float channelPower[]);
    virtual unsigned long millis();

    bool runFile(std::string filename, std::function<void(const dataEntry_t& data)> updateWebserver = nullptr);
    void runTestData(TestEntry_t data[], int size, std::function<void(const TestEntry_t& act)> checkFunc = nullptr);

    void setChannelPower(float c1, float c2, float c3, float c4);

public:
    float _CalcLimit;
    float _Limit;
    float _ChannelPower[4];
    float _MaxChannelPower[4];

private:
    bool OpenFile(std::string file);
    dataEntry_t* getActualOrNext(bool bNext);

private:
    float _factor;
    timeb _Start;
    std::ifstream _file;
};
