// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IShellyWrapper.h"
#include "RamBuffer.h"
#include <functional>
#include <mutex>
#include <string>

enum class ShellyClientDataType_t : uint16_t {
    Pro3EM,
    PlugS,
    CalulatedLimit,
    MAX,
};

typedef std::function<size_t(uint8_t* buffer, size_t maxLen, size_t alreadySent, size_t fileSize)> ResponseFiller;

class ShellyClientData {
public:
    explicit ShellyClientData(ITimeLapse& timeLapse);
    virtual ~ShellyClientData();
    void init(bool psRam);

    void Update(RamDataType_t type, float value);
    void Update(ShellyClientDataType_t type, std::string value);
    float GetActValue(RamDataType_t type);
    float GetMinValue(RamDataType_t type, time_t lastMillis);
    float GetMaxValue(RamDataType_t type, time_t lastMillis);
    float GetFactoredValue(RamDataType_t type, time_t lastMillis);
    std::string& GetLastData(RamDataType_t type, time_t lastMillis, std::string& result);
    std::string& GetDebug(ShellyClientDataType_t type, std::string& result);
    void BackupAll(size_t& fileSize, ResponseFiller& responseFiller);

private:
    ITimeLapse& _timeLapse;
    std::mutex _mutex;
    RamBuffer* _ramBuffer;
    std::string _Debug[static_cast<uint16_t>(ShellyClientDataType_t::MAX)];
};
