// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClientData.h"
#include "MessageOutput.h"
#include <cfloat>
#include <esp32-hal.h>

ShellyClientData::ShellyClientData()
    : _CircularBufferTime(5000)
    , _Pro3EM(true)
{
}

void ShellyClientData::Init(bool bPro3EM)
{
    _Pro3EM = bPro3EM;
}

void ShellyClientData::Update(float value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    unsigned long now = millis();
    if (_Value != value) {
        _LastChangedTime = now;
    }
    _Value = value;

    _times.unshift(now);
    _values.unshift(value);

    CalculateLowestHighest();
}

void ShellyClientData::CalculateLowestHighest()
{
    time_t min_time = millis() - _CircularBufferTime;

    _Lowest = FLT_MAX;
    _Highest = FLT_MIN;

    for (int i = 0; i < _values.size(); i++) {
        if (_times[i] < min_time) {
            break;
        }
        if (_values[i] < _Lowest) {
            _Lowest = _values[i];
        }
        if (_values[i] > _Highest) {
            _Highest = _values[i];
        }
    }
    if (_Lowest == FLT_MAX) {
        _Lowest = _Value;
    }
    if (_Highest == FLT_MIN) {
        _Highest = _Value;
    }

    if (_Highest - _Lowest > 500) {
        _CircularBufferTime = 12000;
    } else {
        _CircularBufferTime = 9000;
    }

    /*if (_Highest - _Lowest > 150) {
       _CircularBufferTime = 9000;
   } else if (_Highest - _Lowest > 70) {
       _CircularBufferTime = 5000;
   } else {
       _CircularBufferTime = 3000;
   }*/
}

float ShellyClientData::GetActValue()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _Value;
}

float ShellyClientData::GetLowestValue()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _Lowest;
}

float ShellyClientData::GetHighestValue()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _Highest;
}

void ShellyClientData::SetLastValue(float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_values.size() > 0) {
        _values.shift();
        _values.unshift(value);

        CalculateLowestHighest();
    }
}

uint32_t ShellyClientData::GetUpdateTime()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _LastChangedTime;
}

uint32_t ShellyClientData::GetCircularBufferTime()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _CircularBufferTime;
}
