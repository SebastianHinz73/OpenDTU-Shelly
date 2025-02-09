// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <functional>

#include <ctime>
#include <stdint.h>

enum ShellyClientType_t : uint16_t {
    Pro3EM,
    PlugS,
    Combined,
};

#pragma pack(2)
typedef struct
{
    ShellyClientType_t type;
    time_t time;     // TODO millis kompatibel? unsinged?
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

class RamBuffer {
public:
    RamBuffer(uint8_t* buffer, size_t size);
    void PowerOnInitialize();

    void writeValue(ShellyClientType_t type, time_t time, float value);
    dataEntry_t* getLastEntry(ShellyClientType_t type);
    void forAllEntries(ShellyClientType_t type, time_t lastMillis, const std::function<void(dataEntry_t*)>& doDataEntry);

private:
    int toIndex(const dataEntry_t* entry) { return entry - _header->start; }

private:
    dataEntryHeader_t* _header;
    size_t _elements;
};
