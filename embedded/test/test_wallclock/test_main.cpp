#include <unity.h>

#include "time/WallClock.h"

using ftime::fromUnixEpoch;
using ftime::minutesSinceMidnight;
using ftime::toUnixEpoch;
using ftime::WallClock;

namespace {

void test_minutes_since_midnight() {
    WallClock wc;
    wc.hour = 10;
    wc.minute = 30;
    TEST_ASSERT_EQUAL_UINT16(630, minutesSinceMidnight(wc));

    wc.hour = 0;
    wc.minute = 0;
    TEST_ASSERT_EQUAL_UINT16(0, minutesSinceMidnight(wc));

    wc.hour = 23;
    wc.minute = 59;
    TEST_ASSERT_EQUAL_UINT16(1439, minutesSinceMidnight(wc));
}

void test_epoch_known_value() {
    // 2021-01-01 00:00:00 UTC == 1609459200.
    WallClock wc;
    wc.year = 2021;
    wc.month = 1;
    wc.day = 1;
    TEST_ASSERT_EQUAL_UINT32(1609459200u, toUnixEpoch(wc));
}

void test_epoch_roundtrip() {
    WallClock in;
    in.year = 2026;
    in.month = 6;
    in.day = 13;
    in.hour = 20;
    in.minute = 47;
    in.second = 9;

    const WallClock out = fromUnixEpoch(toUnixEpoch(in));
    TEST_ASSERT_EQUAL_UINT16(in.year, out.year);
    TEST_ASSERT_EQUAL_UINT8(in.month, out.month);
    TEST_ASSERT_EQUAL_UINT8(in.day, out.day);
    TEST_ASSERT_EQUAL_UINT8(in.hour, out.hour);
    TEST_ASSERT_EQUAL_UINT8(in.minute, out.minute);
    TEST_ASSERT_EQUAL_UINT8(in.second, out.second);
}

void test_epoch_zero_is_unix_epoch() {
    const WallClock wc = fromUnixEpoch(0);
    TEST_ASSERT_EQUAL_UINT16(1970, wc.year);
    TEST_ASSERT_EQUAL_UINT8(1, wc.month);
    TEST_ASSERT_EQUAL_UINT8(1, wc.day);
    TEST_ASSERT_EQUAL_UINT8(0, wc.hour);
    TEST_ASSERT_EQUAL_UINT8(0, wc.minute);
    TEST_ASSERT_EQUAL_UINT8(0, wc.second);
    TEST_ASSERT_EQUAL_UINT32(0, toUnixEpoch(wc));
}

void test_end_of_day_seconds() {
    // 1970-01-01 23:59:59 == 86399 s.
    WallClock wc;
    wc.year = 1970;
    wc.hour = 23;
    wc.minute = 59;
    wc.second = 59;
    TEST_ASSERT_EQUAL_UINT32(86399u, toUnixEpoch(wc));
}

void test_year_2000_known_value() {
    // 2000-01-01 00:00:00 UTC == 946684800 (century leap-year boundary).
    WallClock wc;
    wc.year = 2000;
    wc.month = 1;
    wc.day = 1;
    TEST_ASSERT_EQUAL_UINT32(946684800u, toUnixEpoch(wc));
}

void test_day_rollover_roundtrip() {
    // One second before midnight rolls into the next day on +1 s.
    WallClock eve;
    eve.year = 2025;
    eve.month = 12;
    eve.day = 31;
    eve.hour = 23;
    eve.minute = 59;
    eve.second = 59;
    const WallClock next = fromUnixEpoch(toUnixEpoch(eve) + 1);
    TEST_ASSERT_EQUAL_UINT16(2026, next.year);
    TEST_ASSERT_EQUAL_UINT8(1, next.month);
    TEST_ASSERT_EQUAL_UINT8(1, next.day);
    TEST_ASSERT_EQUAL_UINT8(0, next.hour);
    TEST_ASSERT_EQUAL_UINT8(0, next.minute);
    TEST_ASSERT_EQUAL_UINT8(0, next.second);
}

void test_high_epoch_roundtrip_2106() {
    // Near the 32-bit unsigned epoch ceiling (year ~2106) must still round-trip.
    const uint32_t epoch = 4000000000u;
    const WallClock wc = fromUnixEpoch(epoch);
    TEST_ASSERT_EQUAL_UINT32(epoch, toUnixEpoch(wc));
}

void test_leap_day_roundtrip() {
    WallClock in;
    in.year = 2024;  // leap year
    in.month = 2;
    in.day = 29;
    in.hour = 12;
    const WallClock out = fromUnixEpoch(toUnixEpoch(in));
    TEST_ASSERT_EQUAL_UINT16(2024, out.year);
    TEST_ASSERT_EQUAL_UINT8(2, out.month);
    TEST_ASSERT_EQUAL_UINT8(29, out.day);
    TEST_ASSERT_EQUAL_UINT8(12, out.hour);
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_minutes_since_midnight);
    RUN_TEST(test_epoch_known_value);
    RUN_TEST(test_epoch_roundtrip);
    RUN_TEST(test_epoch_zero_is_unix_epoch);
    RUN_TEST(test_end_of_day_seconds);
    RUN_TEST(test_year_2000_known_value);
    RUN_TEST(test_day_rollover_roundtrip);
    RUN_TEST(test_high_epoch_roundtrip_2106);
    RUN_TEST(test_leap_day_roundtrip);
    return UNITY_END();
}
