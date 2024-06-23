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
    _Pro3EM.type = ShellyClientType_t::Pro3EM;
    _PlugS.type = ShellyClientType_t::PlugS;
    _Combined.type = ShellyClientType_t::Combined;
}

void ShellyClientData::Update(ShellyClientType_t type, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (type == ShellyClientType_t::Combined) {
        return;
    }
    Data& data = GetData(type);

    unsigned long now = millis();
    if (data.lastValue != value) {
        data.lastChangedTime = now;
    }
    data.lastValue = value;

    data.times.unshift(now);
    data.values.unshift(value);

    CalculateMinMax(data);

    Data& other = GetData(type == ShellyClientType_t::Pro3EM ? ShellyClientType_t::PlugS : ShellyClientType_t::Pro3EM);

    if (data.values.size() > 2 && other.values.size() > 1) {

        // 'other' last time measurement between 'data' last two measurements
        if (data.times[0] >= other.times[0] && other.times[0] >= data.times[1] && data.times[0] > data.times[1]) {
            // linear factor between two values
            float f1 = (other.times[0] - data.times[1]) / (data.times[0] - data.times[1]);
            float interpolatedValue = data.values[0] * f1 + data.values[1] * (1.0 - f1);

            value = other.values[0] + interpolatedValue;
            now = other.times[0];
            if (_Combined.lastValue != value) {
                _Combined.lastChangedTime = now;
            }
            _Combined.lastValue = value;

            _Combined.values.unshift(value);
            _Combined.times.unshift(now);

            CalculateMinMax(_Combined);
        }
    }
}

void ShellyClientData::CalculateMinMax(Data& data)
{
    time_t min_time = millis() - data.minMaxTime;

    data.min = FLT_MAX;
    data.max = FLT_MIN;

    for (int i = 0; i < data.values.size(); i++) {
        if (data.times[i] < min_time) {
            break;
        }
        if (data.values[i] < data.min) {
            data.min = data.values[i];
        }
        if (data.values[i] > data.max) {
            data.max = data.values[i];
        }
    }
    if (data.min == FLT_MAX) {
        data.min = data.lastValue;
    }
    if (data.max == FLT_MIN) {
        data.max = data.lastValue;
    }

    if (data.type == ShellyClientType_t::Pro3EM || data.type == ShellyClientType_t::Combined) {
        if (data.max - data.min > 200) {
            data.minMaxTime = 20000;
        } else if (data.max - data.min > 100) {
            data.minMaxTime = 9000;
        } else {
            data.minMaxTime = 5000;
        }
    } else {
        if (data.max - data.min > 150) {
            data.minMaxTime = 10000;
        } else if (data.max - data.min > 30) {
            data.minMaxTime = 5000;
        } else {
            data.minMaxTime = 3000;
        }
    }
}

ShellyClientData::Data& ShellyClientData::GetData(ShellyClientType_t type)
{
    if (type == ShellyClientType_t::Pro3EM) {
        return _Pro3EM;
    } else if (type == ShellyClientType_t::PlugS) {
        return _PlugS;
    }
    return _Combined;
}

float ShellyClientData::GetActValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return GetData(type).lastValue;
}

float ShellyClientData::GetMinValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return GetData(type).min;
}

float ShellyClientData::GetMaxValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return GetData(type).max;
}

float ShellyClientData::GetFactoredValue(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    float min = GetData(type).min;
    float max = GetData(type).max;

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
    if (_Pro3EM.values.size() > 0) {
        _Pro3EM.values.shift();
        _Pro3EM.values.unshift(value);

        CalculateMinMax(_Pro3EM);
    }
}

uint32_t ShellyClientData::GetUpdateTime(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return GetData(type).lastChangedTime;
}

uint32_t ShellyClientData::GetMinMaxTime(ShellyClientType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return GetData(type).minMaxTime;
}
