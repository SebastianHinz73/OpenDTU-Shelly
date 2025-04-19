// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// #include "RamDataType.h"
// #include <Arduino.h>

class ILimitControlHoymiles {
public:
    virtual ~ILimitControlHoymiles() { }

    virtual bool isReachable() = 0;
    virtual bool sendLimit(float limit) = 0;
    virtual int fetchChannelPower(float channelPower[]) = 0;
    // virtual void Update(RamDataType_t type, float value) = 0;
};
