
#include "RamBuffer.h"
#include <stdio.h>
#include <sys\timeb.h>

#include <unity.h>

class TestLapse : public ITimeLapse {
public:
    virtual ~TestLapse() { }
    virtual unsigned long millis()
    {
        return _cnt++;
    }

private:
    unsigned long _cnt = 0;

    // https://stackoverflow.com/questions/17250932/how-to-get-the-time-elapsed-in-c-in-milliseconds-windows
};

void test_basic(void)
{
    printf("test_basic:\n");

    TestLapse lapse;
    RamBuffer buffer(new uint8_t[4096], 4096, lapse);
    buffer.PowerOnInitialize();
    for (int i = 0; i < 10; i++) {
        buffer.writeValue(RamDataType_t::Pro3EM, i * 100, 100 + i);
        buffer.writeValue(RamDataType_t::PlugS, i * 100 + 2, 200 + i);
    }
    dataEntry_t* e = buffer.getLastEntry(RamDataType_t::Pro3EM);
    TEST_ASSERT_EQUAL(900, e->time);
    TEST_ASSERT_EQUAL(109, e->value);

    e = buffer.getLastEntry(RamDataType_t::PlugS);
    TEST_ASSERT_EQUAL(902, e->time);
    TEST_ASSERT_EQUAL(209, e->value);

    printf("forAllEntries:\n");
    int i = 0;
    buffer.forAllEntries(RamDataType_t::Pro3EM, 1000, [&](dataEntry_t* entry) {
        printf(" %lld, %.1f\r\n", entry->time, entry->value);
        TEST_ASSERT_EQUAL(i * 100, entry->time);
        TEST_ASSERT_EQUAL(100 + i, entry->value);
        i++;
    });

    i = 0;
    buffer.forAllEntries(RamDataType_t::PlugS, 1000, [&](dataEntry_t* entry) {
        printf(" %lld, %.1f\r\n", entry->time, entry->value);
        TEST_ASSERT_EQUAL(i * 100 + 2, entry->time);
        TEST_ASSERT_EQUAL(200 + i, entry->value);
        i++;
    });

    printf("forAllEntriesReverse:\n");
    i = 9;
    buffer.forAllEntriesReverse(RamDataType_t::Pro3EM, 1000, [&](dataEntry_t* entry) {
        printf(" %lld, %.1f\r\n", entry->time, entry->value);
        TEST_ASSERT_EQUAL(i * 100, entry->time);
        TEST_ASSERT_EQUAL(100 + i, entry->value);
        i--;
    });
    i = 9;
    buffer.forAllEntriesReverse(RamDataType_t::PlugS, 1000, [&](dataEntry_t* entry) {
        printf(" %lld, %.1f\r\n", entry->time, entry->value);
        TEST_ASSERT_EQUAL(i * 100 + 2, entry->time);
        TEST_ASSERT_EQUAL(200 + i, entry->value);
        i--;
    });

    TEST_ASSERT_EQUAL(20, buffer.getUsedElements());
}

void test_full(void)
{
    printf("test_full:\n");

    TestLapse lapse;
    RamBuffer buffer(new uint8_t[sizeof(dataEntryHeader_t) + 5 * sizeof(dataEntry_t)], sizeof(dataEntryHeader_t) + 5 * sizeof(dataEntry_t), lapse);
    buffer.PowerOnInitialize();
    for (int i = 0; i < 10; i++) {
        buffer.writeValue(RamDataType_t::Pro3EM, i * 100, 100 + i);
    }

    printf("forAllEntries:\n");
    int i = 6;
    buffer.forAllEntries(RamDataType_t::Pro3EM, 1000, [&](dataEntry_t* entry) {
        printf(" %lld, %.1f\r\n", entry->time, entry->value);
        TEST_ASSERT_EQUAL(i * 100, entry->time);
        i++;
    });

    printf("forAllEntriesReverse:\n");
    i = 9;
    buffer.forAllEntriesReverse(RamDataType_t::Pro3EM, 1000, [&](dataEntry_t* entry) {
        printf(" %lld, %.1f\r\n", entry->time, entry->value);
        TEST_ASSERT_EQUAL(i * 100, entry->time);
        TEST_ASSERT_EQUAL(100 + i, entry->value);
        i--;
    });

    TEST_ASSERT_EQUAL(4, buffer.getUsedElements());
}

void test_time()
{
    printf("test_time:\n");
    TestLapse lapse;
    RamBuffer buffer(new uint8_t[4096], 4096, lapse);
    buffer.PowerOnInitialize();
    for (int i = 0; i < 10; i++) {
        buffer.writeValue(RamDataType_t::Pro3EM, lapse.millis(), 0);
        buffer.writeValue(RamDataType_t::PlugS, lapse.millis(), 0);
    }
    for (int n = 0; n < 1000; n++) {
        lapse.millis();
    }
    for (int i = 0; i < 10; i++) {
        buffer.writeValue(RamDataType_t::Pro3EM, lapse.millis(), 0);
        buffer.writeValue(RamDataType_t::PlugS, lapse.millis(), 0);
    }
    auto now = lapse.millis();
    TEST_ASSERT_EQUAL(now - 2, buffer.getLastEntry(RamDataType_t::Pro3EM)->time);
    TEST_ASSERT_EQUAL(now - 1, buffer.getLastEntry(RamDataType_t::PlugS)->time);

    printf("forAllEntries:\n");
    int i = 0;
    buffer.forAllEntries(RamDataType_t::Pro3EM, 50, [&](dataEntry_t*) {
        i++;
    });
    TEST_ASSERT_EQUAL(i, 10);

    i = 0;
    buffer.forAllEntries(RamDataType_t::PlugS, 50, [&](dataEntry_t*) {
        i++;
    });
    TEST_ASSERT_EQUAL(i, 10);
}

void test_getNextEntry()
{
    printf("test_getNextEntry:\n");

    TestLapse lapse;
    RamBuffer buffer(new uint8_t[4096], 4096, lapse);
    buffer.PowerOnInitialize();
    for (int i = 0; i < 10; i++) {
        buffer.writeValue(RamDataType_t::Pro3EM, i * 100, 100 + i);
        buffer.writeValue(RamDataType_t::PlugS, i * 100 + 2, 200 + i);
    }
    int i = 0;
    dataEntry_t* act = nullptr;
    while (buffer.getNextEntry(act)) {
        printf("act: %d, %.1f\n", (int)act->type, act->value);
        i++;
    }
    TEST_ASSERT_EQUAL(i, 20);
}

// https://medium.com/engineering-iot/unit-testing-on-esp32-with-platformio-a-step-by-step-guide-d33f3241192b

void test_RamBuffer()
{
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    RUN_TEST(test_full);
    RUN_TEST(test_time);
    RUN_TEST(test_getNextEntry);
    UNITY_END();
}
