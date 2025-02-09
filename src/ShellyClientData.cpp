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
    int ramDiskSize = 4096;
    uint8_t* ramDisk = new uint8_t[ramDiskSize];

    _ramBuffer = new RamBuffer(ramDisk, ramDiskSize);
    _ramBuffer->PowerOnInitialize();
}

ShellyClientData::~ShellyClientData()
{
    // TODO change RamBuffer
    delete _ramBuffer;
}

void ShellyClientData::Update(ShellyClientType_t type, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _ramBuffer->writeValue(type, millis(), value);
}

float ShellyClientData::GetActValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);

    dataEntry_t* e = _ramBuffer->getLastEntry(type);
    return e != nullptr ? e->value : 0.0;
}

float ShellyClientData::GetMinValue(ShellyClientType_t type, time_t lastMillis)
{
    std::lock_guard<std::mutex> lock(_mutex);

    float min = FLT_MAX;
    _ramBuffer->forAllEntries(type, lastMillis, [&](dataEntry_t* entry) {
        if (entry->value < min) {
            min = entry->value;
        }
    });
    return min == FLT_MAX ? 0 : min;
}

float ShellyClientData::GetMaxValue(ShellyClientType_t type, time_t lastMillis)
{
    std::lock_guard<std::mutex> lock(_mutex);

    float max = FLT_MIN;
    _ramBuffer->forAllEntries(type, lastMillis, [&](dataEntry_t* entry) {
        if (entry->value > max) {
            max = entry->value;
        }
    });
    return max == FLT_MIN ? 0 : max;
}

float ShellyClientData::GetFactoredValue(ShellyClientType_t type, time_t lastMillis)
{
    std::lock_guard<std::mutex> lock(_mutex);

    float min = FLT_MAX;
    float max = FLT_MIN;
    _ramBuffer->forAllEntries(type, lastMillis, [&](dataEntry_t* entry) {
        if (entry->value < min) {
            min = entry->value;
        }
        if (entry->value > max) {
            max = entry->value;
        }
    });
    
    min = min == FLT_MAX ? 0 : min;
    max = max == FLT_MIN ? 0 : max;
    
    const CONFIG_T& config = Configuration.get();

    float factor = static_cast<float>(config.Shelly.FeedInLevel) / 100.0;
    if (type == ShellyClientType_t::PlugS) {
        factor = 1.0 - factor;
    }
    return min + (max - min) * factor;
}

#if 0
void ShellyClientData::SetLastValue(float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    /*
     if (_Pro3EM.values.size() > 0) {
         _Pro3EM.values.shift();
         _Pro3EM.values.unshift(value);

         CalculateMinMax(_Pro3EM);
     }*/
}

uint32_t ShellyClientData::GetUpdateTime(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return 0;
    // return GetData(type).lastChangedTime;
}

uint32_t ShellyClientData::GetMinMaxTime(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return 0;
    // return GetData(type).minMaxTime;
}
#endif