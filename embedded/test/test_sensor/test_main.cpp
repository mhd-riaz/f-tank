#include <unity.h>

#include "sensor/Ds18b20Math.h"

using sensor::crc8;
using sensor::isPlausibleAquariumCelsius;
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

void test_plausibility_exact_boundaries() {
    // Band is the open interval (-55, 125): the endpoints are NOT plausible.
    TEST_ASSERT_FALSE(isPlausibleCelsius(-55.0f));
    TEST_ASSERT_FALSE(isPlausibleCelsius(125.0f));
    TEST_ASSERT_TRUE(isPlausibleCelsius(-54.9f));
    TEST_ASSERT_TRUE(isPlausibleCelsius(124.9f));
}

void test_power_on_reset_value_85c() {
    // 0x0550 = 85.0C is the DS18B20 power-on/reset register value. It passes the
    // electrical-range check but the defense-in-depth aquarium band rejects it,
    // so a probe that never completed a conversion is treated as a fault.
    const int16_t raw = rawFromScratchpad(0x50, 0x05);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 85.0f, rawToCelsius(raw));
    TEST_ASSERT_TRUE(isPlausibleCelsius(rawToCelsius(raw)));           // electrically OK
    TEST_ASSERT_FALSE(isPlausibleAquariumCelsius(rawToCelsius(raw)));  // impossible for a tank
}

void test_aquarium_band_accepts_normal_water() {
    TEST_ASSERT_TRUE(isPlausibleAquariumCelsius(2.0f));   // cold-water marine
    TEST_ASSERT_TRUE(isPlausibleAquariumCelsius(25.0f));  // tropical
    TEST_ASSERT_TRUE(isPlausibleAquariumCelsius(40.0f));  // hospital tank
}

void test_aquarium_band_boundaries() {
    // Inclusive endpoints bracket the configurable alert range plus headroom.
    TEST_ASSERT_TRUE(isPlausibleAquariumCelsius(-10.0f));
    TEST_ASSERT_TRUE(isPlausibleAquariumCelsius(70.0f));
    TEST_ASSERT_FALSE(isPlausibleAquariumCelsius(-10.1f));
    TEST_ASSERT_FALSE(isPlausibleAquariumCelsius(70.1f));
}

void test_aquarium_band_rejects_sentinels() {
    TEST_ASSERT_FALSE(isPlausibleAquariumCelsius(-127.0f));  // disconnected sentinel
    TEST_ASSERT_FALSE(isPlausibleAquariumCelsius(85.0f));    // power-on-reset value
}

void test_raw_zero_and_min_resolution() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, rawToCelsius(rawFromScratchpad(0x00, 0x00)));
    // Smallest positive step = 1/16 C.
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0625f, rawToCelsius(rawFromScratchpad(0x01, 0x00)));
    // Smallest negative step.
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -0.0625f, rawToCelsius(rawFromScratchpad(0xFF, 0xFF)));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_crc8_known_scratchpad);
    RUN_TEST(test_crc8_detects_corruption);
    RUN_TEST(test_raw_to_celsius_positive);
    RUN_TEST(test_raw_to_celsius_negative);
    RUN_TEST(test_plausibility_band);
    RUN_TEST(test_plausibility_exact_boundaries);
    RUN_TEST(test_power_on_reset_value_85c);
    RUN_TEST(test_aquarium_band_accepts_normal_water);
    RUN_TEST(test_aquarium_band_boundaries);
    RUN_TEST(test_aquarium_band_rejects_sentinels);
    RUN_TEST(test_raw_zero_and_min_resolution);
    return UNITY_END();
}
