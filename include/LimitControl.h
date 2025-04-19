// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ILimitControlHoymiles.h"
#include "IShellyClientData.h"
#include "ITimeLapse.h"
#include "LimitControlCalculation.h"
#include <TaskSchedulerDeclarations.h>

class LimitControlClass : public ILimitControlHoymiles, public ITimeLapse {
public:
    LimitControlClass();
    virtual ~LimitControlClass() { }

    void init(Scheduler& scheduler);
    void loop();

private:
    virtual bool isReachable();
    virtual bool sendLimit(float limit);
    virtual int fetchChannelPower(float channelPower[]);

    virtual unsigned long millis();
private:
    Task _loopTask;
    IShellyClientData& _shellyClientData;
    LimitControlCalculation _calc;
};

extern LimitControlClass LimitControl;
