// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "ShellyWrapperMock.h"
#include <MessageOutput.h>

ShellyWrapperMock::ShellyWrapperMock(float factor)
    : ShellyClientData(*((IShellyWrapper*)this))
    , LimitControlCalculation(*this, *this)
    , _factor(factor)
{
    ftime(&_Start);
}

ShellyWrapperMock::~ShellyWrapperMock()
{
}

void ShellyWrapperMock::run()
{
    auto lastCalc = millis();
    auto lastData = millis();
    float lastValue = 0;

    OpenFile("test\\ShellyData\\shelly_data3.bin");
    MessageOutput.printf("OpenFile\r\n");

    while (_file.is_open() && !_file.eof()) {
        // frames from file ?
        auto now = millis();

        for (dataEntry_t* act = getActualOrNext(false); act != nullptr; act = getActualOrNext(true)) {
            if (act->type != RamDataType_t::Pro3EM) {
                continue;
            }

            if (act->time > now) {
                break;
            }
            ShellyClientData::Update(act->type, act->value);
            lastData = now;
            lastValue = act->value;

            fflush(stdout);
        }

        // update
        if (now - lastData > TASK_SECOND) {
            lastData = now;
            ShellyClientData::Update(RamDataType_t::Pro3EM, lastValue);
        }

        // Calculate ?
        if (now - lastCalc > TASK_SECOND) {
            lastCalc = now;
            LimitControlCalculation::loop();
        }
    }
}

bool ShellyWrapperMock::OpenFile(std::string file)
{
    _file.open(file, std::fstream::binary);
    if (!_file.is_open()) {
        return false;
    }
    // L"test\\ShellyData\\shelly_data.bin"
    /// std::ifstream f1(file, std::fstream::binary);

    return true;
}

dataEntry_t* ShellyWrapperMock::getActualOrNext(bool bNext)
{
    static dataEntry_t entry {};
    static bool bInit = false;
    if (!bInit || bNext) {
        bInit = true;

        if (!_file.is_open() || _file.eof()) {
            return nullptr;
        }

        uint16_t u16;
        uint32_t u32;
        float f32;

        _file.read((char*)&u16, sizeof(uint16_t));
        _file.read((char*)&u32, sizeof(uint32_t));
        _file.read((char*)&f32, sizeof(float));

        entry.type = (RamDataType_t)u16;
        entry.time = u32;
        entry.value = f32;
    }
    return &entry;
}

bool ShellyWrapperMock::sendLimit(float /*limit*/)
{
    return true;
}

int ShellyWrapperMock::fetchChannelPower(float channelPower[])
{
    return 0;
}

unsigned long ShellyWrapperMock::millis()
{
    struct timeb now;
    ftime(&now);
    unsigned long diff = (unsigned long)(1000.0 * (now.time - _Start.time) + (now.millitm - _Start.millitm));
    diff *= _factor;
    return diff;
}
