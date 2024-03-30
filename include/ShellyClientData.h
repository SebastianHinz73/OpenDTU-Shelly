// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <CircularBuffer.h>
#include <mutex>

#define LAST_DATA_ENTRIES 40

class ShellyClientData {
public:
    ShellyClientData();
    void Init(bool bPro3EM);
    void Update(float value);
    float GetActValue();
    float GetLowestValue();
    float GetHighestValue();
    void SetLastValue(float value);
    uint32_t GetUpdateTime();
    uint32_t GetCircularBufferTime();

private:
    void CalculateLowestHighest();

private:
    std::mutex _mutex;

    CircularBuffer<time_t, LAST_DATA_ENTRIES> _times; // seconds since 1970
    CircularBuffer<float, LAST_DATA_ENTRIES> _values;

    float _Value;
    float _Lowest;
    float _Highest;
    uint32_t _LastChangedTime;
    uint32_t _CircularBufferTime;
    bool _Pro3EM;
};
