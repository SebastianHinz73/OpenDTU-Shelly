// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Sebastian Hinz
 */

#include "Configuration.h"
#include "LimitControlCalculation.h"
#include "ShellyWrapperMock.h"
#include <MessageOutput.h>
#include <unity.h>

void test_Limit1(void)
{
    ShellyWrapperMock shellyWrapperMock(1);
    shellyWrapperMock.run();

    TEST_ASSERT_EQUAL(32, 32);
}

// https://medium.com/engineering-iot/unit-testing-on-esp32-with-platformio-a-step-by-step-guide-d33f3241192b

void test_LimitControl()
{
    UNITY_BEGIN();
    RUN_TEST(test_Limit1);
    UNITY_END();
}
