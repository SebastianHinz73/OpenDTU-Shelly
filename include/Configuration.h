// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#ifdef ARDUINO
#include "PinMapping.h"
#include "config_t.h"
#include <TaskSchedulerDeclarations.h>
#include <condition_variable>
#include <cstdint>
#include <mutex>

class ConfigurationClass {
public:
    void init(Scheduler& scheduler);
    bool read();
    bool write();
    void migrate();
    CONFIG_T const& get();

    class WriteGuard {
    public:
        WriteGuard();
        CONFIG_T& getConfig();
        ~WriteGuard();

    private:
        std::unique_lock<std::mutex> _lock;
    };

    WriteGuard getWriteGuard();

    INVERTER_CONFIG_T* getFreeInverterSlot();
    INVERTER_CONFIG_T* getInverterConfig(const uint64_t serial);
    void deleteInverterById(const uint8_t id);

private:
    void loop();

    Task _loopTask;
};

#else
#include "config_t.h"

class ConfigurationClass {
public:
    CONFIG_T const& get();
};
#endif

extern ConfigurationClass Configuration;
