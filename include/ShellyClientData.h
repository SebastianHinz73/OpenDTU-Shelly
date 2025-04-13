// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "RamBuffer.h"
#include <Arduino.h>
#include <mutex>

enum class ShellyClientDataType_t : uint16_t {
    Pro3EM,
    PlugS,
    CalulatedLimit,
    MAX,
};

class ShellyClientData {
public:
    ShellyClientData();
    ~ShellyClientData();

    void Update(RamDataType_t type, float value);
    void Update(ShellyClientDataType_t type, String value);
    float GetActValue(RamDataType_t type);
    float GetMinValue(RamDataType_t type, time_t lastMillis);
    float GetMaxValue(RamDataType_t type, time_t lastMillis);
    float GetFactoredValue(RamDataType_t type, time_t lastMillis);
    String& GetLastData(RamDataType_t type, time_t lastMillis, String& result);
    String& GetDebug(ShellyClientDataType_t type, String& result);

private:
    std::mutex _mutex;
    RamBuffer* _ramBuffer;
    String _Debug[(uint16_t)ShellyClientDataType_t::MAX];
};
