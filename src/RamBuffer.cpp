// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "RamBuffer.h"
#include "MessageOutput.h"

RamBuffer::RamBuffer(uint8_t* buffer, size_t size)
    : _header(reinterpret_cast<dataEntryHeader_t*>(buffer))
    , _elements((size - sizeof(dataEntryHeader_t)) / sizeof(dataEntry_t))
{
    // On reset: _header, _cache and _cacheSize is set. The values in PSRAM are not changes/deleted.
    // On power on: do additional initialisation
    // https://www.esp32.com/viewtopic.php?t=35063
}

void RamBuffer::PowerOnInitialize()
{
    _header->start = reinterpret_cast<dataEntry_t*>(&_header[1]);
    _header->first = _header->start;
    _header->last = _header->start;
    _header->end = &_header->start[_elements];
}

void RamBuffer::writeValue(RamDataType_t type, time_t time, float value)
{
    _header->last->type = type;
    _header->last->time = time;
    _header->last->value = value;
    // MessageOutput.printf("writeValue: ## %d: 0x%x, (%d, %05.2f)\r\n", toIndex(_header->last), _header->last->type, _header->last->time, _header->last->value);

    _header->last++;

    // last on end -> begin with start
    if (_header->last == _header->end) {
        _header->last = _header->start;
    }

    // last overwrites first -> increase first
    if (_header->last == _header->first) {
        _header->first++;
        if (_header->first == _header->end) {
            _header->first = _header->start;
        }
    }
}

dataEntry_t* RamBuffer::getLastEntry(RamDataType_t type)
{
    dataEntry_t* act = _header->last;

    if (_header->last < _header->first) // buffer full
    {
        while (act > _header->start) {
            act--;
            if (act->type == type) {
                return act;
            }
        }
        act = _header->end;
    }

    while (act > _header->first) {
        act--;
        if (act->type == type) {
            return act;
        }
    }
    return nullptr;
}

void RamBuffer::forAllEntriesReverse(RamDataType_t type, time_t lastMillis, const std::function<void(dataEntry_t*)>& doDataEntry)
{
    dataEntry_t* act = _header->last;

    time_t startTime = millis() - lastMillis;
    startTime = startTime < 0 ? 0 : startTime;

    if (_header->last < _header->first) // buffer full
    {
        while (act > _header->start) {
            act--;
            if (act->time < startTime) {
                return;
            }
            if (act->type == type) {
                doDataEntry(act);
            }
        }
        act = _header->end;
    }
    while (act > _header->first) {
        act--;
        if (act->time < startTime) {
            return;
        }
        if (act->type == type) {
            doDataEntry(act);
        }
    }
}

void RamBuffer::forAllEntries(RamDataType_t type, time_t lastMillis, const std::function<void(dataEntry_t*)>& doDataEntry)
{
    dataEntry_t* act = toStartEntry(type, lastMillis);
    if (act == nullptr) {
        return;
    }

    for (int i = 0; i < 2; i++) {
        while (act < _header->end) {
            // end check
            if (act == _header->last) {
                return;
            }

            // type check
            if (type == act->type) {
                doDataEntry(act);
            }
            act++;
        }
        act = _header->start; // start again with _header->start
    }
}

dataEntry_t* RamBuffer::toStartEntry(RamDataType_t type, time_t lastMillis)
{
    time_t startTime = millis() - lastMillis;

    dataEntry_t* act = _header->last;
    dataEntry_t* found = nullptr;

    if (_header->last < _header->first) // buffer full
    {
        while (act > _header->start) {
            act--;
            if (act->time < startTime) {
                return found;
            }
            if (act->type == type) {
                found = act;
            }
        }
        act = _header->end;
    }
    while (act > _header->first) {
        act--;
        if (act->time < startTime) {
            return found;
        }
        if (act->type == type) {
            found = act;
        }
    }

    return found;
}
