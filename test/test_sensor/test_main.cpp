#include <unity.h>

#include "sensor/Ds18b20Math.h"

using sensor::crc8;
using sensor::isPlausibleCelsius;
using sensor::rawFromScratchpad;
using sensor::rawToCelsius;

namespace {

void test_crc8_known_scratchpad() {
    // Valid DS18B20 scratchpad; byte 8 is the Dallas CRC of bytes 0-7.
    uint8_t sp[9] = {0x91, 0x01, 0x4B, 0x46, 0x7F, 0xFF, 0x0F, 0x10, 0x25};
    TEST_ASSERT_EQUAL_UINT8(sp[8], crc8(sp, 8));
}

void test_crc8_detects_corruption() {
    uint8_t sp[9] = {0x91, 0x01, 0x4B, 0x46, 0x7F, 0xFF, 0x0F, 0x10, 0x25};
    sp[2] ^= 0xFF;  // corrupt a byte
    TEST_ASSERT_NOT_EQUAL(sp[8], crc8(sp, 8));
}

void test_raw_to_celsius_positive() {
    // 0x0191 = 401 -> 401 * 0.0625 = 25.0625°C
    const int16_t raw = rawFromScratchpad(0x91, 0x01);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0625f, rawToCelsius(raw));
}

void test_raw_to_celsius_negative() {
    // -10.125°C -> raw = -162 (0xFF5E)
    const int16_t raw = rawFromScratchpad(0x5E, 0xFF);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.125f, rawToCelsius(raw));
}

void test_plausibility_band() {
    TEST_ASSERT_TRUE(isPlausibleCelsius(25.0f));
    TEST_ASSERT_FALSE(isPlausibleCelsius(-127.0f));
    TEST_ASSERT_FALSE(isPlausibleCelsius(200.0f));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_crc8_known_scratchpad);
    RUN_TEST(test_crc8_detects_corruption);
    RUN_TEST(test_raw_to_celsius_positive);
    RUN_TEST(test_raw_to_celsius_negative);
    RUN_TEST(test_plausibility_band);
    return UNITY_END();
}
