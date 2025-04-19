// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "LimitControl.h"
#include "ShellyClient.h"
#include <Hoymiles.h>
#include <cfloat>

LimitControlClass LimitControl;

LimitControlClass::LimitControlClass()
    : _loopTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&LimitControlClass::loop, this))
    , _shellyClientData(ShellyClient.getShellyData())
{
}

void LimitControlClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void LimitControlClass::loop()
{
    _calc.loop(_shellyClientData, *this, *this);
}

bool LimitControlClass::isReachable()
{
    auto inv = Hoymiles.getInverterByPos(0);
    return inv != nullptr && inv->isReachable();
}

bool LimitControlClass::sendLimit(float limit)
{
    auto inv = Hoymiles.getInverterByPos(0);
    return inv && inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent);
}

int LimitControlClass::fetchChannelPower(float channelPower[])
{
    int channelCnt = 0;
    auto inv = Hoymiles.getInverterByPos(0);
    if (inv) {
        String limit = "(";
        for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
            if (inv->Statistics()->getStringMaxPower(c) > 0) {
                auto power = inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                channelPower[c] = power;
                channelCnt++;
                limit += String(static_cast<int>(power)) + ",";
            }
        }
        limit += ")";
    }
    return channelCnt;
}

unsigned long LimitControlClass::millis()
{
    return millis();
}
