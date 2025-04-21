// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

class ILimitControlHoymiles {
public:
    virtual ~ILimitControlHoymiles() { }

    virtual bool isReachable() = 0;
    virtual bool sendLimit(float limit) = 0;
    virtual int fetchChannelPower(float channelPower[]) = 0;
};
