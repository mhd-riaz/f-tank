#include <unity.h>

#include "network/ReconnectBackoff.h"

using network::backoffDelayMs;

namespace {

void test_backoff_first_attempt_is_base() {
    TEST_ASSERT_EQUAL_UINT32(1000, backoffDelayMs(0, 1000, 60000));
}

void test_backoff_doubles_each_attempt() {
    TEST_ASSERT_EQUAL_UINT32(2000, backoffDelayMs(1, 1000, 60000));
    TEST_ASSERT_EQUAL_UINT32(4000, backoffDelayMs(2, 1000, 60000));
    TEST_ASSERT_EQUAL_UINT32(8000, backoffDelayMs(3, 1000, 60000));
}

void test_backoff_clamps_to_max() {
    TEST_ASSERT_EQUAL_UINT32(60000, backoffDelayMs(10, 1000, 60000));
    TEST_ASSERT_EQUAL_UINT32(60000, backoffDelayMs(20, 1000, 60000));
}

void test_backoff_never_exceeds_max_even_small_steps() {
    TEST_ASSERT_EQUAL_UINT32(5000, backoffDelayMs(8, 5000, 5000));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_backoff_first_attempt_is_base);
    RUN_TEST(test_backoff_doubles_each_attempt);
    RUN_TEST(test_backoff_clamps_to_max);
    RUN_TEST(test_backoff_never_exceeds_max_even_small_steps);
    return UNITY_END();
}
