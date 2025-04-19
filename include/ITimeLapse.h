// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#ifndef ARDUINO
#define TASK_SECOND 1000UL
#endif

class ITimeLapse {
public:
    virtual ~ITimeLapse() { }
    virtual unsigned long millis() = 0;
};
