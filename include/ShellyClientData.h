// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <CircularBuffer.h>
#include <mutex>

#define LAST_DATA_ENTRIES 20

class ShellyClientData {
public:
    void Update(float value, const char* text);
    float GetActValue();
    float GetLowestValue();
    float GetHighestValue();

private:
    std::mutex _mutex;

    CircularBuffer<time_t, LAST_DATA_ENTRIES> _times; // seconds since 1970
    CircularBuffer<float, LAST_DATA_ENTRIES> _values;

    float _Value;
    float _Lowest;
    float _Highest;
};
