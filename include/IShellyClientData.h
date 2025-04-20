// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "RamDataType.h"
#include <functional>
#include <string>

enum class ShellyClientDataType_t : uint16_t {
    Pro3EM,
    PlugS,
    CalulatedLimit,
    MAX,
};

typedef std::function<size_t(uint8_t* buffer, size_t maxLen, size_t alreadySent, size_t fileSize)> ResponseFiller;

class IShellyClientData {
public:
    virtual ~IShellyClientData() { }

    virtual void Update(RamDataType_t type, float value) = 0;
    virtual void Update(ShellyClientDataType_t type, std::string value) = 0;
    virtual float GetActValue(RamDataType_t type) = 0;
    virtual float GetMinValue(RamDataType_t type, time_t lastMillis) = 0;
    virtual float GetMaxValue(RamDataType_t type, time_t lastMillis) = 0;
    virtual float GetFactoredValue(RamDataType_t type, time_t lastMillis) = 0;
    virtual std::string& GetLastData(RamDataType_t type, time_t lastMillis, std::string& result) = 0;
    virtual std::string& GetDebug(ShellyClientDataType_t type, std::string& result) = 0;
    virtual void BackupAll(size_t& fileSize, ResponseFiller& responseFiller) = 0;
};
