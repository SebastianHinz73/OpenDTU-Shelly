// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

enum class RamDataType_t : uint16_t {
    Pro3EM,
    Pro3EM_Min,
    Pro3EM_Max,
    PlugS,
    PlugS_Min,
    PlugS_Max,
    CalulatedLimit,
    Limit,
    MAX,
};
