// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#ifdef UNIT_TEST

#include "Configuration.h"

CONFIG_T config;

CONFIG_T const& ConfigurationClass::get()
{
    return config;
}

ConfigurationClass Configuration;

int WinMain()
{
    return 0;
}
#endif
