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

    _ramBuffer = new RamBuffer(ramDisk, ramDiskSize, nullptr, 0);
    _ramBuffer->PowerOnInitialize();

    //    _Pro3EM.type = ShellyClientType_t::Pro3EM;
    //    _PlugS.type = ShellyClientType_t::PlugS;
    //    _Combined.type = ShellyClientType_t::Combined;
}

ShellyClientData::~ShellyClientData()
{
    // TODO change RamBuffer
    delete _ramBuffer;
}
float hack1 = 0;
float hack2 = 0;

void ShellyClientData::Update(ShellyClientType_t type, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (type == ShellyClientType_t::Combined) {
        return;
    }
    if (type == ShellyClientType_t::Pro3EM) {
        hack1 = value;
    } else {
        hack2 = value;
    }
    _ramBuffer->writeValue(type, millis(), value);
}

/*

ShellyClientData::Data& ShellyClientData::GetData(ShellyClientType_t type)
{
    if (type == ShellyClientType_t::Pro3EM) {
        return _Pro3EM;
    } else if (type == ShellyClientType_t::PlugS) {
        return _PlugS;
    }
    return _Combined;
}
*/
float ShellyClientData::GetActValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    //  return GetData(type).lastValue;

    if (type == ShellyClientType_t::Pro3EM) {
        return hack1;
    }

    return hack2;
}

float ShellyClientData::GetMinValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    // return GetData(type).min;
    return 0;
}

float ShellyClientData::GetMaxValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    // return GetData(type).max;
    return 0;
}

float ShellyClientData::GetFactoredValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    float min = 0.0; // GetData(type).min;
    float max = 0.0; // GetData(type).max;

    const CONFIG_T& config = Configuration.get();

    float factor = static_cast<float>(config.Shelly.FeedInLevel) / 100.0;
    if (type == ShellyClientType_t::PlugS) {
        factor = 1.0 - factor;
    }
    return min + (max - min) * factor;
}

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
