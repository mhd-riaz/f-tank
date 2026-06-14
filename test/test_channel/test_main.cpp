#include <unity.h>

#include "channel/RelayPolarity.h"

using channel::deEnergizedLevel;
using channel::gpioLevelFor;
using channel::RelayPolarity;

namespace {

void test_active_high_mapping() {
    // Active-high: energize -> HIGH(true), off -> LOW(false).
    TEST_ASSERT_TRUE(gpioLevelFor(true, RelayPolarity::kActiveHigh));
    TEST_ASSERT_FALSE(gpioLevelFor(false, RelayPolarity::kActiveHigh));
}

void test_active_low_mapping() {
    // Active-low: energize -> LOW(false), off -> HIGH(true).
    TEST_ASSERT_FALSE(gpioLevelFor(true, RelayPolarity::kActiveLow));
    TEST_ASSERT_TRUE(gpioLevelFor(false, RelayPolarity::kActiveLow));
}

void test_deenergized_safe_level() {
    // De-energized level must equal the "off" mapping for each polarity.
    TEST_ASSERT_FALSE(deEnergizedLevel(RelayPolarity::kActiveHigh));
    TEST_ASSERT_TRUE(deEnergizedLevel(RelayPolarity::kActiveLow));
}

void test_polarity_does_not_change_intent() {
    // Same logical intent, opposite electrical level across polarities (NFR-11).
    TEST_ASSERT_NOT_EQUAL(gpioLevelFor(true, RelayPolarity::kActiveHigh),
                          gpioLevelFor(true, RelayPolarity::kActiveLow));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_active_high_mapping);
    RUN_TEST(test_active_low_mapping);
    RUN_TEST(test_deenergized_safe_level);
    RUN_TEST(test_polarity_does_not_change_intent);
    return UNITY_END();
}
