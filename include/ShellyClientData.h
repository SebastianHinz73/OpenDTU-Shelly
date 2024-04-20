// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <CircularBuffer.h>
#include <mutex>

#define LAST_DATA_ENTRIES 40

enum ShellyClientType_t {
    Pro3EM,
    PlugS,
    Combined,
};

class ShellyClientData {
public:
    ShellyClientData();
    void Update(ShellyClientType_t type, float value);
    float GetActValue(ShellyClientType_t type);
    float GetMinValue(ShellyClientType_t type);
    float GetMaxValue(ShellyClientType_t type);
    float GetFactoredValue(ShellyClientType_t type);
    void SetLastValue(float value);
    uint32_t GetUpdateTime(ShellyClientType_t type);
    uint32_t GetMinMaxTime(ShellyClientType_t type);

private:
    struct Data {
        Data()
        {
            minMaxTime = 5000;
        }
        CircularBuffer<time_t, LAST_DATA_ENTRIES> times; // seconds since 1970
        CircularBuffer<float, LAST_DATA_ENTRIES> values;

        float lastValue;
        float min;
        float max;

        uint32_t lastChangedTime;
        uint32_t minMaxTime;
        ShellyClientType_t type;
    };
    void CalculateMinMax(Data& data);
    Data& GetData(ShellyClientType_t type);

private:
    std::mutex _mutex;

    Data _Pro3EM;
    Data _PlugS;
    Data _Combined;
};
