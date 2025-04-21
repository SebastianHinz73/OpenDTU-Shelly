// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IShellyWrapper.h"
#include "LimitControlCalculation.h"
#include "ShellyClientData.h"
#include <fstream>
#include <mutex>

class ShellyWrapperMock : public ShellyClientData, public LimitControlCalculation, public IShellyWrapper {
public:
    ShellyWrapperMock(float factor = 1.0);
    virtual ~ShellyWrapperMock();

    virtual bool isReachable() { return true; }
    virtual bool sendLimit(float limit);
    virtual int fetchChannelPower(float channelPower[]);
    virtual unsigned long millis();

    void run();

private:
    bool OpenFile(std::string file);
    dataEntry_t* getActualOrNext(bool bNext);

private:
    float _factor;
    timeb _Start;
    std::ifstream _file;
};
