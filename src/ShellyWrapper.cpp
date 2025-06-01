// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "ShellyWrapper.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>

ShellyWrapperClass ShellyWrapper;

ShellyWrapperClass::ShellyWrapperClass()
    : ShellyClientData(*((IShellyWrapper*)this))
    , LimitControlCalculation(*this, *this)
    , _loopFetchTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&ShellyWrapperClass::loopFetch, this))
    , _loopCalcTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&ShellyWrapperClass::loopCalc, this))
    , _nativeDebug(false)
{
}

void ShellyWrapperClass::init(Scheduler& scheduler)
{
    ShellyClientData::init(ESP.getPsramSize() > 0);

    scheduler.addTask(_loopFetchTask);
    _loopFetchTask.enable();

    scheduler.addTask(_loopCalcTask);
    _loopCalcTask.enable();

    _Pro3EM.ShellyType = RamDataType_t::Pro3EM;
    _Pro3EM.MaxInterval = 5000;
    _PlugS.ShellyType = RamDataType_t::PlugS;
    _PlugS.MaxInterval = 5000;
}

void ShellyWrapperClass::loopFetch()
{
    while (WiFi.status() != WL_CONNECTED) {
        return;
    }
    // if (!SunPosition.isDayPeriod()) {
    //     return;
    // }
    const CONFIG_T& config = Configuration.get();

    HandleWebsocket(_Pro3EM, config.Shelly.Hostname_Pro3EM, ShellyWrapperClass::EventsPro3EM);
    HandleWebsocket(_PlugS, config.Shelly.Hostname_PlugS, ShellyWrapperClass::EventsPlugS);
}

void ShellyWrapperClass::loopCalc()
{
    if (!_nativeDebug) {
        LimitControlCalculation::loop();
    }
}

void ShellyWrapperClass::HandleWebsocket(WebSocketData& data, const char* hostname, std::function<void(WStype_t type, uint8_t* payload, size_t length)> cbEvent)
{
    const CONFIG_T& config = Configuration.get();
    unsigned long nowMillis = millis();

    bool bDelete = data.Host.compare(hostname) != 0; // hostname changed in configuration
    bDelete |= strlen(hostname) == 0; // IP deleted in configuration
    bDelete |= !config.Shelly.ShellyEnable; // shelly disabled

    bool bNew = bDelete && strlen(hostname) > 0; // deleted and new hostname valid
    bNew &= config.Shelly.ShellyEnable; // && shelly enable

    if (bDelete && data.Client != nullptr) {
        MessageOutput.printf("Delete WebSocketClient. %s\r\n", data.Host.c_str());
        delete data.Client;
        data.Client = nullptr;
        data.Host = "";
    }

    if (bNew && data.Client == nullptr) {
        MessageOutput.printf("Add WebSocketClient %s\r\n", hostname);
        data.Client = new WebSocketsClient();
        data.Client->begin(hostname, 80, "/rpc");
        data.Client->onEvent(cbEvent);
        data.Client->setReconnectInterval(2000);
        data.Host = hostname;
        data.LastTime = nowMillis;
        data.UpdatedTime = data.LastTime;
    }

    if (data.Connected) {
        if (nowMillis - data.LastTime > data.MaxInterval) {
            std::string send = "{\"id\":0, \"src\":\"user_";
            send += NetworkSettings.localIP().toString().c_str();
            send += "\", \"method\":\"Shelly.GetStatus\"}";
            if (data.ShellyType == RamDataType_t::Pro3EM) {
                send = send.replace(6, 1, "1");
            }
            data.Client->sendTXT(send.c_str());

            std::lock_guard<std::mutex> lock(_mutex);
            data.LastTime = nowMillis;
            data.UpdatedTime = data.LastTime;
        } else if (nowMillis - data.UpdatedTime > 1000) {
            ShellyClientData::Update(data.ShellyType, data.LastValue);
            data.UpdatedTime += 1000;
        }
    }

    if (data.Client != nullptr) {
        data.Client->loop();
    }
}

void ShellyWrapperClass::EventsPro3EM(WStype_t type, uint8_t* payload, size_t length)
{
    ShellyWrapper.Events(ShellyWrapper._Pro3EM, type, payload, length);
    MessageOutput.printf("EventsPro3EM\r\n");
}

void ShellyWrapperClass::EventsPlugS(WStype_t type, uint8_t* payload, size_t length)
{
    ShellyWrapper.Events(ShellyWrapper._PlugS, type, payload, length);
}

void ShellyWrapperClass::Events(WebSocketData& data, WStype_t type, uint8_t* payload, size_t length)
{
    switch (type) {
    case WStype_DISCONNECTED:
        MessageOutput.printf("[WSc] Disconnected!\r\n");
        data.Connected = false;
        break;
    case WStype_CONNECTED:
        MessageOutput.printf("[WSc] Connected to url: %s\r\n", payload);
        data.Connected = true;
        break;
    case WStype_TEXT: {
        // MessageOutput.printf("[WSc] WStype_TEXT %d -> %s length: %u\r\n", data.ShellyType == RamDataType_t::Pro3EM, payload, length);
        auto ParseDouble = [&](const char* search, double& result) {
            // e.g. ...ull,"total_act_power":225.658,"total
            // e.g. ...ue, "apower":0.0, "volta
            char* key = strstr(reinterpret_cast<char*>(payload), search);
            if (key != nullptr) {
                key += strlen(search);
                result = atof(key);
                return true;
            }
            return false;
        };

        if (!_nativeDebug) {
            if (data.ShellyType == RamDataType_t::Pro3EM) {
                if (ParseDouble("\"total_act_power\":", data.LastValue)) {
                    ShellyClientData::Update(data.ShellyType, data.LastValue);
                    break;
                }
            } else {
                if (ParseDouble("\"apower\":", data.LastValue)) {
                    ShellyClientData::Update(data.ShellyType, data.LastValue);
                    break;
                }
            }
            if (strstr(reinterpret_cast<char*>(payload), "opendtu_shelly_debug") != nullptr) {
                _nativeDebug = true;
                break;
            }
        } else {
            double value;
            if (ParseDouble("::Pro3EM:", ShellyWrapper._Pro3EM.LastValue)) {
                ShellyClientData::Update(RamDataType_t::Pro3EM, ShellyWrapper._Pro3EM.LastValue);
                break;
            }
            if (ParseDouble("::Pro3EM_Min:", value)) {
                ShellyClientData::Update(RamDataType_t::Pro3EM_Min, value);
                break;
            }
            if (ParseDouble("::Pro3EM_Max:", value)) {
                ShellyClientData::Update(RamDataType_t::Pro3EM_Max, value);
                break;
            }
            if (ParseDouble("::PlugS:", ShellyWrapper._PlugS.LastValue)) {
                ShellyClientData::Update(RamDataType_t::PlugS, ShellyWrapper._PlugS.LastValue);
                break;
            }
            if (ParseDouble("::PlugS_Min:", value)) {
                ShellyClientData::Update(RamDataType_t::PlugS_Min, value);
                break;
            }
            if (ParseDouble("::PlugS_Max:", value)) {
                ShellyClientData::Update(RamDataType_t::PlugS_Max, value);
                break;
            }
            if (ParseDouble("::CalulatedLimit:", value)) {
                ShellyClientData::Update(RamDataType_t::CalulatedLimit, value);
                break;
            }
            if (ParseDouble("::Limit:", value)) {
                ShellyClientData::Update(RamDataType_t::Limit, value);
                break;
            }
        }
    } break;
    case WStype_BIN:
        MessageOutput.printf("[WSc] get binary length: %u\r\n", length);
        break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
        break;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    data.LastTime = millis();
    data.UpdatedTime = data.LastTime;
}

bool ShellyWrapperClass::isReachable()
{
    auto inv = Hoymiles.getInverterByPos(0);
    return inv != nullptr && inv->isReachable();
}

bool ShellyWrapperClass::sendLimit(float limit)
{
    auto inv = Hoymiles.getInverterByPos(0);
    return inv && inv->sendActivePowerControlRequest(limit, PowerLimitControlType::AbsolutNonPersistent);
}

int ShellyWrapperClass::fetchChannelPower(float channelPower[])
{
    int channelCnt = 0;
    auto inv = Hoymiles.getInverterByPos(0);
    if (inv) {
        String limit = "(";
        for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
            if (inv->Statistics()->getStringMaxPower(c) > 0) {
                auto power = inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                channelPower[c] = power;
                channelCnt++;
                limit += String(static_cast<int>(power)) + ",";
            }
        }
        limit += ")";
    }
    return channelCnt;
}

uint32_t ShellyWrapperClass::millis()
{
    return ::millis();
}
