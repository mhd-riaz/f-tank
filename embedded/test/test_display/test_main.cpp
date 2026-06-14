#include <unity.h>

#include <cstring>

#include "display/ScreenFormat.h"

using display::formatChannelStates;
using display::formatDate;
using display::formatTemperature;
using display::formatTime;
using ftime::WallClock;

namespace {

WallClock sample() {
    WallClock wc;
    wc.year = 2026;
    wc.month = 6;
    wc.day = 13;
    wc.hour = 9;
    wc.minute = 5;
    wc.second = 7;
    return wc;
}

void test_format_time() {
    char buf[12];
    formatTime(buf, sizeof(buf), sample());
    TEST_ASSERT_EQUAL_STRING("09:05:07", buf);
}

void test_format_date() {
    char buf[12];
    formatDate(buf, sizeof(buf), sample());
    TEST_ASSERT_EQUAL_STRING("13/06/2026", buf);
}

void test_format_temperature_valid() {
    char buf[16];
    formatTemperature(buf, sizeof(buf), 25.14f, true);
    TEST_ASSERT_EQUAL_STRING("Temp: 25.1 C", buf);
}

void test_format_temperature_rounding() {
    char buf[16];
    formatTemperature(buf, sizeof(buf), 25.16f, true);
    TEST_ASSERT_EQUAL_STRING("Temp: 25.2 C", buf);
}

void test_format_temperature_invalid() {
    char buf[16];
    formatTemperature(buf, sizeof(buf), -127.0f, false);
    TEST_ASSERT_EQUAL_STRING("Temp: --.- C", buf);
}

void test_format_channel_states() {
    char buf[16];
    bool states[4] = {true, false, true, false};
    formatChannelStates(buf, sizeof(buf), states, 4);
    TEST_ASSERT_EQUAL_STRING("Ch:1010", buf);
}

void test_format_channel_states_truncates_safely() {
    char buf[6];  // "Ch:" + room for only 2 digits + null
    bool states[8] = {true, true, true, true, true, true, true, true};
    formatChannelStates(buf, sizeof(buf), states, 8);
    TEST_ASSERT_TRUE(strlen(buf) < sizeof(buf));  // never overflows
}

void test_format_temperature_negative() {
    char buf[16];
    formatTemperature(buf, sizeof(buf), -3.24f, true);
    TEST_ASSERT_EQUAL_STRING("Temp: -3.2 C", buf);  // sign + one decimal preserved
}

void test_format_time_midnight() {
    char buf[12];
    WallClock wc;  // all-zero -> 1970-01-01 00:00:00
    formatTime(buf, sizeof(buf), wc);
    TEST_ASSERT_EQUAL_STRING("00:00:00", buf);
}

void test_format_channel_states_zero_count() {
    char buf[16];
    bool states[1] = {true};
    formatChannelStates(buf, sizeof(buf), states, 0);
    TEST_ASSERT_EQUAL_STRING("Ch:", buf);  // header only, no digits
}

void test_ascii_only_no_degree_symbol() {
    char buf[16];
    formatTemperature(buf, sizeof(buf), 25.0f, true);
    for (size_t i = 0; buf[i] != '\0'; ++i) {
        TEST_ASSERT_TRUE((static_cast<unsigned char>(buf[i]) & 0x80) == 0);
    }
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_format_time);
    RUN_TEST(test_format_date);
    RUN_TEST(test_format_temperature_valid);
    RUN_TEST(test_format_temperature_rounding);
    RUN_TEST(test_format_temperature_invalid);
    RUN_TEST(test_format_channel_states);
    RUN_TEST(test_format_channel_states_truncates_safely);
    RUN_TEST(test_format_temperature_negative);
    RUN_TEST(test_format_time_midnight);
    RUN_TEST(test_format_channel_states_zero_count);
    RUN_TEST(test_ascii_only_no_degree_symbol);
    return UNITY_END();
}
