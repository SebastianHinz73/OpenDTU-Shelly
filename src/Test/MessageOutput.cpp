// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#ifdef UNIT_TEST

#include "MessageOutput.h"
#include <cstdarg>
#include <stdio.h>

MessageOutputClass MessageOutput;

void MessageOutputClass::printf(const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    vfprintf(stdout, format, arg);
    va_end(arg);
}
#endif
