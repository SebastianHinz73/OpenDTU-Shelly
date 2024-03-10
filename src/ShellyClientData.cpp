// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "ShellyClientData.h"
#include "MessageOutput.h"
#include <cfloat>
#include <esp32-hal.h>

void ShellyClientData::Update(float value, const char* text)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _Value = value;

    _times.unshift(millis());
    _values.unshift(value);

    time_t min_time = millis() - 5000;
    _Lowest = FLT_MAX;
    _Highest = FLT_MIN;

    // MessageOutput.printf("Update %s ", text);

    for (int i = 0; i < _values.size(); i++) {
        if (_times[i] < min_time) {
            break;
        }
        // MessageOutput.printf("%.1f ", _values[i]);

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
    // MessageOutput.printf("=> [%.1f, %.1f]\r\n", _Lowest, _Highest);
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
