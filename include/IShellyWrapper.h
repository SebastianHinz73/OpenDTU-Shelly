// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>

#ifndef ARDUINO
#define TASK_SECOND 1000UL
#endif

class ITimeLapse {
public:
    virtual ~ITimeLapse() { }
    virtual uint32_t millis() = 0;
};

class IShellyWrapper : public ITimeLapse {
public:
    virtual ~IShellyWrapper() { }

    virtual bool isReachable() = 0;
    virtual bool sendLimit(float limit) = 0;
    virtual int fetchChannelPower(float channelPower[]) = 0;
};
