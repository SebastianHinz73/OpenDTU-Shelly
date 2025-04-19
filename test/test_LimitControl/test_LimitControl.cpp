/*
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/

#include "Configuration.h"
#include "LimitControlCalculation.h"
#include <unity.h>

LimitControlCalculation limitCalc;

void test_abc(void)
{
    TEST_ASSERT_EQUAL(32, 32);
}

// https://medium.com/engineering-iot/unit-testing-on-esp32-with-platformio-a-step-by-step-guide-d33f3241192b

void test_LimitControl()
{
    UNITY_BEGIN();
    RUN_TEST(test_abc);
    UNITY_END();
}
