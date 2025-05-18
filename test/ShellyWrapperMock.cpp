// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "ShellyWrapperMock.h"
#include <MessageOutput.h>
#include <iostream>
#include <thread>

ShellyWrapperMock::ShellyWrapperMock(float factor)
    : ShellyClientData(*((IShellyWrapper*)this))
    , LimitControlCalculation(*this, *this)
    , _factor(factor)
    , _lastTimePlugS(0)
    , _lastValuePlugS(0)
    , _offsetPro3em(0)
{
    ftime(&_Start);
    init(false); // ShellyClientData
}

ShellyWrapperMock::~ShellyWrapperMock()
{
}

bool ShellyWrapperMock::runFile(std::string filename, std::function<void(RamDataType_t type, float value)> updateWebserver, bool onlyView)
{
    auto lastCalc = millis();
    auto lastData = millis();

    if (!OpenFile(filename)) {
        std::cout << "Can not open %s" << filename << std::endl;
        return false;
    }
    std::cout << "Open file " << filename << std::endl;

    while (_file.is_open() && !_file.eof()) {
        // frames from file ?
        auto now = millis();

        for (dataEntry_t* act = getActualOrNext(false); act != nullptr; act = getActualOrNext(true)) {
            if (act->type != RamDataType_t::Pro3EM) {
                continue;
            }
            auto act_value = act->value - _lastValuePlugS + _offsetPro3em;

            if (act->time > now) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                break;
            }
            if (updateWebserver != nullptr) {
                updateWebserver(RamDataType_t::Pro3EM, act_value);
            }

            ShellyClientData::Update(RamDataType_t::Pro3EM, act_value);
            lastData = now;
        }

        if (onlyView) {
            fflush(stdout);
            continue;
        }

        // update
        if (now - lastData > TASK_SECOND) {
            lastData = now;
            ShellyClientData::Update(RamDataType_t::Pro3EM, ShellyClientData::GetActValue(RamDataType_t::Pro3EM));
        }
        if (now - _lastTimePlugS > TASK_SECOND) {
            _lastTimePlugS = now;
            ShellyClientData::Update(RamDataType_t::PlugS, ShellyClientData::GetActValue(RamDataType_t::PlugS));
        }

        // Calculate ?
        if (now - lastCalc > TASK_SECOND) {
            lastCalc = now;
            LimitControlCalculation::loop();

            if (updateWebserver != nullptr) {
                updateWebserver(RamDataType_t::Pro3EM_Min, ShellyClientData::GetActValue(RamDataType_t::Pro3EM_Min));
                updateWebserver(RamDataType_t::Pro3EM_Max, ShellyClientData::GetActValue(RamDataType_t::Pro3EM_Max));
                updateWebserver(RamDataType_t::PlugS, ShellyClientData::GetActValue(RamDataType_t::PlugS));
                updateWebserver(RamDataType_t::CalulatedLimit, ShellyClientData::GetActValue(RamDataType_t::CalulatedLimit));
                updateWebserver(RamDataType_t::Limit, ShellyClientData::GetActValue(RamDataType_t::Limit));
            }
        }
        fflush(stdout);
    }
    return true;
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

void ShellyWrapperMock::runTestData(TestEntry_t data[], int size, std::function<void(const TestEntry_t& act)> checkFunc)
{
    auto lastCalc = millis();
    auto lastData = millis();

    for (TestEntry_t* act = data; act < &data[size]; act++) {
        auto now = millis();
        bool newDataProcessed = false;

        for (; act < &data[size]; act++) {
            if (act->time * 1000 > now) {
                act--;
                break;
            }

            float plugS = ShellyClientData::GetActValue(RamDataType_t::PlugS);
            ShellyClientData::Update(RamDataType_t::Pro3EM, act->value - plugS);
            lastData = now;

            newDataProcessed = true;
        }

        // update
        if (now - lastData > TASK_SECOND) {
            lastData = now;

            ShellyClientData::Update(RamDataType_t::Pro3EM, ShellyClientData::GetActValue(RamDataType_t::Pro3EM));
            ShellyClientData::Update(RamDataType_t::PlugS, ShellyClientData::GetActValue(RamDataType_t::PlugS));
        }

        // Calculate ?
        if (now - lastCalc > TASK_SECOND) {
            lastCalc = now;
            LimitControlCalculation::loop();
            _CalcLimit = ShellyClientData::GetActValue(RamDataType_t::CalulatedLimit);
        }
        if (newDataProcessed && checkFunc && act < &data[size]) {
            checkFunc(*act);
        }
        fflush(stdout);
    }
}

void ShellyWrapperMock::setChannelPower(float c1, float c2, float c3, float c4)
{
    _MaxChannelPower[0] = c1;
    _MaxChannelPower[1] = c2;
    _MaxChannelPower[2] = c3;
    _MaxChannelPower[3] = c4;
    for (int i = 0; i < 4; i++) {
        _ChannelPower[i] = _MaxChannelPower[i];
    }
}

bool ShellyWrapperMock::sendLimit(float limit)
{
    MessageOutput.printf("##############sendLimit %f\r\n", limit);

    _Limit = limit;

    limit /= 4;
    float sum = 0;
    for (int i = 0; i < 4; i++) {
        _ChannelPower[i] = std::min(limit, _MaxChannelPower[i]);
        sum += _ChannelPower[i];
    }
    ShellyClientData::Update(RamDataType_t::PlugS, sum);
    ShellyClientData::Update(RamDataType_t::Limit, sum);

    _lastTimePlugS = millis();
    _lastValuePlugS = sum;
    return true;
}

int ShellyWrapperMock::fetchChannelPower(float channelPower[])
{
    for (int i = 0; i < 4; i++) {
        channelPower[i] = _ChannelPower[i];
    }
    return 4;
}

unsigned long ShellyWrapperMock::millis()
{
    struct timeb now;
    ftime(&now);
    unsigned long diff = (unsigned long)(1000.0 * (now.time - _Start.time) + (now.millitm - _Start.millitm));
    diff *= _factor;
    return diff;
}
