#include "ShellyClientDataMock.h"
#include <Config_t.h>
#include <Configuration.h>
#include <filesystem>
#include <fstream>
// #include <iostream>
// #include <stdio.h>

#define FLT_MAX 100000.

unsigned long ShellyClientDataMock::millis()
{
    // https://stackoverflow.com/questions/17250932/how-to-get-the-time-elapsed-in-c-in-milliseconds-windows
    return _myTime;
}

bool ShellyClientDataMock::OpenFile(std::string file)
{
    _file.open(file, std::fstream::binary);
    if (!_file.is_open()) {
        return false;
    }
    // L"test\\ShellyData\\shelly_data.bin"
    /// std::ifstream f1(file, std::fstream::binary);

    return true;
}

dataEntry_t* ShellyClientDataMock::getActualOrNext(bool bNext)
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

bool ShellyClientDataMock::loop()
{
    _myTime += 100;
    auto now = millis();

    dataEntry_t* act = getActualOrNext(false);
    if (act == nullptr) {
        return false;
    }
    while (act->time < now) {
        if (act->type == RamDataType_t::Pro3EM) {
            _ramBuffer->writeValue(act->type, act->time, act->value);
            printf("Pro3EM: %1.f\n", act->value);
        }
        act = getActualOrNext(true);
        if (act == nullptr) {
            return false;
        }
    }
    return true;
}

ShellyClientDataMock::ShellyClientDataMock()
{
    int ramDiskSize = 4096; // 7 * 60 Sekunden * 10 Bytes = 4200
    uint8_t* ramDisk = new uint8_t[ramDiskSize];

    _ramBuffer = new RamBuffer(ramDisk, ramDiskSize, *this);
    _ramBuffer->PowerOnInitialize();
}

ShellyClientDataMock::~ShellyClientDataMock()
{
    // TODO(shi) change RamBuffer
    delete _ramBuffer;
}

void ShellyClientDataMock::Update(RamDataType_t type, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _ramBuffer->writeValue(type, millis(), value);
}

void ShellyClientDataMock::Update(ShellyClientDataType_t type, std::string value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (type >= ShellyClientDataType_t::Pro3EM && type < ShellyClientDataType_t::MAX) {
        uint16_t i = (uint16_t)type;

        if (_Debug[i].length() > 30) {
            _Debug[i] = "";
        }
        _Debug[i] += value;
    }
}

float ShellyClientDataMock::GetActValue(RamDataType_t type)
{
    std::lock_guard<std::mutex> lock(_mutex);

    dataEntry_t* e = _ramBuffer->getLastEntry(type);
    return e != nullptr ? e->value : 0.0;
}

float ShellyClientDataMock::GetMinValue(RamDataType_t type, time_t lastMillis)
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

float ShellyClientDataMock::GetMaxValue(RamDataType_t type, time_t lastMillis)
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

float ShellyClientDataMock::GetFactoredValue(RamDataType_t type, time_t lastMillis)
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

std::string& ShellyClientDataMock::GetLastData(RamDataType_t type, time_t lastMillis, std::string& result)
{
    std::lock_guard<std::mutex> lock(_mutex);

    int cnt = 0;
    result = "[ ";
    _ramBuffer->forAllEntries(type, lastMillis, [&](dataEntry_t* entry) {
        if (cnt == 0) {
            cnt++;
        } else {
            result += std::string(",");
        }
        result += std::string("{\"x\": ") + std::to_string(static_cast<float>(entry->time) / TASK_SECOND) + std::string(",\"y\": ") + std::to_string(entry->value) + std::string("}");
    });
    result += " ]";

    return result;
}

void ShellyClientDataMock::BackupAll(size_t&, ResponseFiller&)
{
}

std::string& ShellyClientDataMock::GetDebug(ShellyClientDataType_t type, std::string& result)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (type >= ShellyClientDataType_t::Pro3EM && type < ShellyClientDataType_t::MAX) {
        uint16_t i = (uint16_t)type;
        result = _Debug[i].c_str();
        _Debug[i] = "";
    }

    return result;
}

bool ShellyClientDataMock::sendLimit(float limit)
{
    return true;
}

int ShellyClientDataMock::fetchChannelPower(float channelPower[])
{
    return 0;
}
