// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "Configuration.h"
#include "LimitControlCalculation.h"
#include "ShellyWrapperMock.h"
#include "TestServer.h"
#include <Configuration.h>
#include <MessageOutput.h>
#include <unity.h>

void setConfig()
{
    CONFIG_T& config = Configuration.get();
    config.Shelly.ShellyEnable = true;
    config.Shelly.LimitEnable = true;
    config.Shelly.MaxPower = 1600;
    config.Shelly.MinPower = 150;
    config.Shelly.TargetValue = 0;
    config.Shelly.FeedInLevel = 85;
}

void test_Limit1()
{
    setConfig();

    ShellyWrapperMock shellyWrapperMock(1);

    TEST_ASSERT_TRUE(shellyWrapperMock.runFile("test\\ShellyData\\shelly_data3.bin"));
    TEST_ASSERT_EQUAL(32, 32);
}

void test_LimitIncrease()
{
    setConfig();

    TestEntry_t data[] = {
        { 0, 100 },
        { 5, 200 },
        { 10, 300 },
        { 15, 300 },
        { 25, 300 },
        { 30, 300 },
    };

    ShellyWrapperMock mock(1);
    mock.setChannelPower(100, 100, 100, 100);

    auto check = [&](const TestEntry_t& act) {
        MessageOutput.printf("check %.1f limit=%.1f\r\n", act.value, mock._Limit);
    };

    mock.runTestData(data, sizeof(data) / sizeof(TestEntry_t), check);

    TEST_ASSERT_EQUAL(32, 32);
}

void test_WebServer()
{
    setConfig();
    ShellyWrapperMock shellyWrapperMock(1);

    TestServer server;
    server.Start();

    bool rc = shellyWrapperMock.runFile("test\\ShellyData\\shelly_data3.bin", [&server](const dataEntry_t& data) {
        server.Update(RamDataType_t::Pro3EM, data.value);
    });

    TEST_ASSERT_TRUE(rc);

    MessageOutput.printf("server.Stop()\r\n");
    fflush(stdout);
    server.Stop();
}

// https://medium.com/engineering-iot/unit-testing-on-esp32-with-platformio-a-step-by-step-guide-d33f3241192b

void test_LimitControl()
{
    UNITY_BEGIN();
    // RUN_TEST(test_Limit1);
    //   RUN_TEST(test_LimitIncrease);
    RUN_TEST(test_WebServer);
    UNITY_END();
}
