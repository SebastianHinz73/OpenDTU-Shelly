// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ITimeLapse.h"
#include <sys\timeb.h>

class TimeLapseMock : public ITimeLapse {
public:
    TimeLapseMock(float factor = 1.0);
    virtual ~TimeLapseMock() { }
    virtual unsigned long millis();

private:
    timeb _Start;
    float _factor;
};
