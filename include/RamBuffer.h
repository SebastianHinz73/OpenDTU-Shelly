// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

enum ShellyClientType_t : uint16_t {
    Pro3EM,
    PlugS,
    Combined,
};

#pragma pack(2)
typedef struct
{
    ShellyClientType_t type;
    time_t time;
    float value;
} dataEntry_t; // 2 + 4 + 4 => 10 Bytes

typedef struct
{
    uint32_t id;
    dataEntry_t* start;
    dataEntry_t* first;
    dataEntry_t* last;
    dataEntry_t* end;
} dataEntryHeader_t;

#pragma pack()

#define RAMBUFFER_HEADER_ID 0x12345678

class RamBuffer {
public:
    RamBuffer(uint8_t* buffer, size_t size, uint8_t* cache, size_t cacheSize);
    void PowerOnInitialize();
    bool IntegrityCheck();

    void writeValue(ShellyClientType_t type, time_t time, float value);
    bool getEntry(ShellyClientType_t type, time_t time, dataEntry_t*& act);

    size_t getTotalElements() { return _elements; }
    size_t getUsedElements() { return _header->last >= _header->first ? _header->last - _header->first : _elements; }

private:
    int toIndex(const dataEntry_t* entry) { return entry - _header->start; }

public:
    dataEntryHeader_t* _header;
    size_t _elements;
    uint8_t* _cache;
    size_t _cacheSize;
};
