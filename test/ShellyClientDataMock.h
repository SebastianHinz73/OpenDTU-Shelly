// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ILimitControlHoymiles.h"
#include "IShellyClientData.h"
#include "ITimeLapse.h"
#include "RamBuffer.h"
#include <fstream>
#include <mutex>

class ShellyClientDataMock : public IShellyClientData, public ILimitControlHoymiles {
public:
    ShellyClientDataMock(ITimeLapse& timeLapse);
    virtual ~ShellyClientDataMock();

    virtual void Update(RamDataType_t type, float value);
    virtual void Update(ShellyClientDataType_t type, std::string value);
    virtual float GetActValue(RamDataType_t type);
    virtual float GetMinValue(RamDataType_t type, time_t lastMillis);
    virtual float GetMaxValue(RamDataType_t type, time_t lastMillis);
    virtual float GetFactoredValue(RamDataType_t type, time_t lastMillis);
    virtual std::string& GetLastData(RamDataType_t type, time_t lastMillis, std::string& result);
    virtual std::string& GetDebug(ShellyClientDataType_t type, std::string& result);
    virtual void BackupAll(size_t& fileSize, ResponseFiller& responseFiller);

    virtual bool isReachable() { return true; }
    virtual bool sendLimit(float limit);
    virtual int fetchChannelPower(float channelPower[]);

    bool OpenFile(std::string file);
    bool loop();

private:
    dataEntry_t* getActualOrNext(bool bNext);

private:
    std::mutex _mutex;
    RamBuffer* _ramBuffer;
    std::string _Debug[(uint16_t)ShellyClientDataType_t::MAX];
    std::ifstream _file;
    ITimeLapse& _timeLapse;
};
