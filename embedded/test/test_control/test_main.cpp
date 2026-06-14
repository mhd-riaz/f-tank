#include <unity.h>

#include "channel/ChannelController.h"
#include "control/ControlLogic.h"

using control::isMaskableIndex;
using control::maskBit;
using control::resolveChannelOutput;
using control::setMaskBit;
using control::shouldCutHeater;
using control::staggerElapsed;

namespace {

void test_stagger_gate() {
    TEST_ASSERT_FALSE(staggerElapsed(100, 100, 40));  // no time passed
    TEST_ASSERT_FALSE(staggerElapsed(139, 100, 40));  // 39 < 40
    TEST_ASSERT_TRUE(staggerElapsed(140, 100, 40));   // exactly the interval
    TEST_ASSERT_TRUE(staggerElapsed(1000, 100, 40));  // well past
}

void test_stagger_gate_millis_overflow_safe() {
    // last just below the 32-bit wrap, now just after it: unsigned subtraction
    // yields the true elapsed time (no false "not elapsed").
    const uint32_t last = 0xFFFFFFF0u;
    const uint32_t now = 0x00000020u;  // 0x30 (48) ms later across the wrap
    TEST_ASSERT_TRUE(staggerElapsed(now, last, 40));
    TEST_ASSERT_FALSE(staggerElapsed(0xFFFFFFF5u, last, 40));  // 5 ms, no wrap
}

void test_maskable_index_bounds() {
    TEST_ASSERT_TRUE(isMaskableIndex(0));
    TEST_ASSERT_TRUE(isMaskableIndex(channel::kMaxChannels - 1));  // 15
    TEST_ASSERT_FALSE(isMaskableIndex(channel::kMaxChannels));     // 16 == mask width
    TEST_ASSERT_FALSE(isMaskableIndex(200));
}

void test_set_and_read_mask_bit() {
    uint16_t mask = 0;
    mask = setMaskBit(mask, 0, true);
    mask = setMaskBit(mask, 15, true);
    TEST_ASSERT_TRUE(maskBit(mask, 0));
    TEST_ASSERT_TRUE(maskBit(mask, 15));
    TEST_ASSERT_FALSE(maskBit(mask, 1));
    TEST_ASSERT_EQUAL_UINT16(0x8001, mask);

    mask = setMaskBit(mask, 0, false);
    TEST_ASSERT_FALSE(maskBit(mask, 0));
    TEST_ASSERT_EQUAL_UINT16(0x8000, mask);
}

void test_mask_out_of_range_is_safe_noop() {
    uint16_t mask = 0x00FF;
    // Setting an out-of-range bit must not shift past the mask or change it.
    TEST_ASSERT_EQUAL_UINT16(0x00FF, setMaskBit(mask, 16, true));
    TEST_ASSERT_EQUAL_UINT16(0x00FF, setMaskBit(mask, 99, true));
    // Reading an out-of-range bit is false, never UB.
    TEST_ASSERT_FALSE(maskBit(mask, 16));
    TEST_ASSERT_FALSE(maskBit(mask, 255));
}

void test_resolve_channel_output() {
    TEST_ASSERT_TRUE(resolveChannelOutput(true, false));    // scheduled on, not forced
    TEST_ASSERT_FALSE(resolveChannelOutput(false, false));  // scheduled off
    TEST_ASSERT_FALSE(resolveChannelOutput(true, true));    // force-off overrides schedule
    TEST_ASSERT_FALSE(resolveChannelOutput(false, true));   // force-off, already off
}

void test_should_cut_heater() {
    TEST_ASSERT_FALSE(shouldCutHeater(false, false));  // all clear
    TEST_ASSERT_TRUE(shouldCutHeater(true, false));    // sensor fault -> fail safe
    TEST_ASSERT_TRUE(shouldCutHeater(false, true));    // over-temp -> cut
    TEST_ASSERT_TRUE(shouldCutHeater(true, true));     // both
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_stagger_gate);
    RUN_TEST(test_stagger_gate_millis_overflow_safe);
    RUN_TEST(test_maskable_index_bounds);
    RUN_TEST(test_set_and_read_mask_bit);
    RUN_TEST(test_mask_out_of_range_is_safe_noop);
    RUN_TEST(test_resolve_channel_output);
    RUN_TEST(test_should_cut_heater);
    return UNITY_END();
}
