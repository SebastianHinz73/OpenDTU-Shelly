// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "RamBuffer.h"
#include <Arduino.h>
#include <mutex>

#define LAST_DATA_ENTRIES 40

class ShellyClientData {
public:
    ShellyClientData();
    ~ShellyClientData();

    void Update(RamDataType_t type, float value);
    float GetActValue(RamDataType_t type);
    float GetMinValue(RamDataType_t type, time_t lastMillis);
    float GetMaxValue(RamDataType_t type, time_t lastMillis);
    float GetFactoredValue(RamDataType_t type, time_t lastMillis);
    void GetLastData(RamDataType_t type, time_t lastMillis, String& result);

private:
    std::mutex _mutex;
    RamBuffer* _ramBuffer;
};
