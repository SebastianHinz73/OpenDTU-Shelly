// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IShellyClientData.h"
#include "IShellyWrapper.h"
#include "RamBuffer.h"
#include <mutex>
#include <string.h>

class ShellyClientData : public IShellyClientData {
public:
    ShellyClientData(ITimeLapse& timeLapse);
    virtual ~ShellyClientData();

    virtual void Update(RamDataType_t type, float value);
    virtual void Update(ShellyClientDataType_t type, std::string value);
    virtual float GetActValue(RamDataType_t type);
    virtual float GetMinValue(RamDataType_t type, time_t lastMillis);
    virtual float GetMaxValue(RamDataType_t type, time_t lastMillis);
    virtual float GetFactoredValue(RamDataType_t type, time_t lastMillis);
    virtual std::string& GetLastData(RamDataType_t type, time_t lastMillis, std::string& result);
    virtual std::string& GetDebug(ShellyClientDataType_t type, std::string& result);
    virtual void BackupAll(size_t& fileSize, ResponseFiller& responseFiller);

private:
    ITimeLapse& _timeLapse;
    std::mutex _mutex;
    RamBuffer* _ramBuffer;
    std::string _Debug[(uint16_t)ShellyClientDataType_t::MAX];
};
