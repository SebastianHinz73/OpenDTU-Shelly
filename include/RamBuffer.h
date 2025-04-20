// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ITimeLapse.h"
#include "RamDataType.h"
#include <functional>

#include <ctime>
#include <stdint.h>

#pragma pack(2)
typedef struct
{
    RamDataType_t type;
    time_t time; // TODO(shi) millis kompatibel? unsinged?
    float value;
} dataEntry_t; // 2 + 4 + 4 => 10 Bytes

typedef struct
{
    dataEntry_t* start;
    dataEntry_t* first;
    dataEntry_t* last;
    dataEntry_t* end;
} dataEntryHeader_t;

#pragma pack()

#define RAMBUFFER_HEADER_ID 0x12345678

typedef std::function<void(dataEntry_t*)> DoDataEntry;

class RamBuffer {
public:
    RamBuffer(uint8_t* buffer, size_t size, ITimeLapse& timeLapse);
    void PowerOnInitialize();

    void writeValue(RamDataType_t type, time_t time, float value);
    dataEntry_t* getLastEntry(RamDataType_t type);
    void forAllEntriesReverse(RamDataType_t type, time_t lastMillis, const DoDataEntry& doDataEntry);
    void forAllEntries(RamDataType_t type, time_t lastMillis, const DoDataEntry& doDataEntry);
    bool getNextEntry(dataEntry_t*& act);
    size_t getUsedElements() const { return _header->last >= _header->first ? _header->last - _header->first : _elements; }

private:
    int toIndex(const dataEntry_t* entry) { return entry - _header->start; }
    dataEntry_t* toStartEntry(RamDataType_t type, time_t lastMillis);

private:
    dataEntryHeader_t* _header;
    size_t _elements;
    ITimeLapse& _timeLapse;
};
