// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */
#include "TimeLapseMock.h"
#include "MessageOutput.h"

#include <stdio.h>

TimeLapseMock::TimeLapseMock(float factor)
    : _factor(factor)
{
    ftime(&_Start);
}
// https://stackoverflow.com/questions/17250932/how-to-get-the-time-elapsed-in-c-in-milliseconds-windows

unsigned long TimeLapseMock::millis()
{
    struct timeb now;
    ftime(&now);
    unsigned long diff = (unsigned long)(1000.0 * (now.time - _Start.time) + (now.millitm - _Start.millitm));
    diff *= _factor;
    return diff;
}
