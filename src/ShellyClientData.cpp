// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "ShellyClientData.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include <cfloat>

ShellyClientData::ShellyClientData(ITimeLapse& timeLapse)
    : _timeLapse(timeLapse)
{
    int ramDiskSize = 4096; // 7 * 60 Sekunden * 10 Bytes = 4200
    uint8_t* ramDisk = new uint8_t[ramDiskSize];

    _ramBuffer = new RamBuffer(ramDisk, ramDiskSize, _timeLapse);
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

    auto now = _timeLapse.millis();
    if (type == RamDataType_t::Pro3EM) {
        MessageOutput.printf("## Pro3EM %.1f, %lu\r\n", value, now);
    }

    _ramBuffer->writeValue(type, now, value);
}

void ShellyClientData::Update(ShellyClientDataType_t type, std::string value)
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

std::string& ShellyClientData::GetLastData(RamDataType_t type, time_t lastMillis, std::string& result)
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

void ShellyClientData::BackupAll(size_t& fileSize, ResponseFiller& responseFiller)
{
    _mutex.lock();

    fileSize = _ramBuffer->getUsedElements() * sizeof(dataEntry_t);

    static dataEntry_t* act;
    act = nullptr;

    responseFiller = [&](uint8_t* buffer, size_t maxLen, size_t /*alreadySent*/, size_t /*fileSize*/) -> size_t {
        size_t ret = 0;
        size_t maxCnt = maxLen / sizeof(dataEntry_t);

        for (size_t cnt = 0; cnt < maxCnt; cnt++) {
            if (!_ramBuffer->getNextEntry(act)) {
                break;
            }
            memcpy(&buffer[cnt * sizeof(dataEntry_t)], act, sizeof(dataEntry_t));
            ret += sizeof(dataEntry_t);
        }

        if (ret == 0) {
            _mutex.unlock();
        }
        return ret;
    };
}

std::string& ShellyClientData::GetDebug(ShellyClientDataType_t type, std::string& result)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (type >= ShellyClientDataType_t::Pro3EM && type < ShellyClientDataType_t::MAX) {
        uint16_t i = (uint16_t)type;
        result = _Debug[i].c_str();
        _Debug[i] = "";
    }

    return result;
}
