// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClientData.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include <cfloat>
#include <esp32-hal.h>

ShellyClientData::ShellyClientData()
{
    int ramDiskSize = 4096; // 7 * 60 Sekunden * 10 Bytes = 4200
    uint8_t* ramDisk = new uint8_t[ramDiskSize];

    _ramBuffer = new RamBuffer(ramDisk, ramDiskSize);
    _ramBuffer->PowerOnInitialize();
}

ShellyClientData::~ShellyClientData()
{
    // TODO(shi) change RamBuffer
    delete _ramBuffer;
}

void ShellyClientData::Update(RamDataType_t type, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _ramBuffer->writeValue(type, millis(), value);
}

float ShellyClientData::GetActValue(RamDataType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);

    dataEntry_t* e = _ramBuffer->getLastEntry(type);
    return e != nullptr ? e->value : 0.0;
}

float ShellyClientData::GetMinValue(RamDataType_t type, time_t lastMillis)
{
    std::lock_guard<std::mutex> lock(_mutex);

    float min = FLT_MAX;
    _ramBuffer->forAllEntriesReverse(type, lastMillis, [&](dataEntry_t* entry) {
        if (entry->value < min) {
            min = entry->value;
        }
    });
    return (min == FLT_MAX) ? 0 : min;
}

float ShellyClientData::GetMaxValue(RamDataType_t type, time_t lastMillis)
{
    std::lock_guard<std::mutex> lock(_mutex);

    float max = -FLT_MAX;

    _ramBuffer->forAllEntriesReverse(type, lastMillis, [&](dataEntry_t* entry) {
        if (entry->value > max) {
            max = entry->value;
        }
    });
    return (max == -FLT_MAX) ? 0 : max;
}

float ShellyClientData::GetFactoredValue(RamDataType_t type, time_t lastMillis)
{
    std::lock_guard<std::mutex> lock(_mutex);

    float min = FLT_MAX;
    float max = -FLT_MAX;
    _ramBuffer->forAllEntriesReverse(type, lastMillis, [&](dataEntry_t* entry) {
        if (entry->value < min) {
            min = entry->value;
        }
        if (entry->value > max) {
            max = entry->value;
        }
    });

    min = min == FLT_MAX ? 0 : min;
    max = max == -FLT_MAX ? 0 : max;

    const CONFIG_T& config = Configuration.get();

    float factor = static_cast<float>(config.Shelly.FeedInLevel) / 100.0;
    if (type == RamDataType_t::PlugS) {
        factor = 1.0 - factor;
    }
    return min + (max - min) * factor;
}

String& ShellyClientData::GetLastData(RamDataType_t type, time_t lastMillis, String& result)
{
    std::lock_guard<std::mutex> lock(_mutex);

    int cnt = 0;
    result = "[ ";
    _ramBuffer->forAllEntries(type, lastMillis, [&](dataEntry_t* entry) {
        if (cnt == 0) {
            cnt++;
        } else {
            result += String(",");
        }
        result += String("{\"x\": ") + static_cast<float>(entry->time) / TASK_SECOND + String(",\"y\": ") + entry->value + String("}");
    });
    result += " ]";

    return result;
}
