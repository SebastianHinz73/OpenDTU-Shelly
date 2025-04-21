// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "Configuration.h"
#include "LimitControlCalculation.h"
#include "ShellyClientDataMock.h"
#include "TimeLapseMock.h"
#include <unity.h>

void test_abc(void)
{
    TimeLapseMock timeLapse(1);
    ShellyClientDataMock shellyMock(timeLapse);
    LimitControlCalculation limitCalc(shellyMock, timeLapse);

    TEST_ASSERT_EQUAL(true, shellyMock.OpenFile("test\\ShellyData\\shelly_data3.bin"));

    while (shellyMock.loop()) {
        limitCalc.loop(shellyMock);
    }

    TEST_ASSERT_EQUAL(32, 32);
}

// https://medium.com/engineering-iot/unit-testing-on-esp32-with-platformio-a-step-by-step-guide-d33f3241192b

void test_LimitControl()
{
    UNITY_BEGIN();
    RUN_TEST(test_abc);
    UNITY_END();
}
