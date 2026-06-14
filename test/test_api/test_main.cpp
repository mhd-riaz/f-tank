#include <unity.h>

#include <cstring>

#include "api/ApiValidation.h"
#include "api/AuthToken.h"

namespace {

void test_constant_time_equal_match() {
    TEST_ASSERT_TRUE(api::constantTimeEquals("abc123", "abc123", 49));
}

void test_constant_time_equal_mismatch() {
    TEST_ASSERT_FALSE(api::constantTimeEquals("abc123", "abc124", 49));
    TEST_ASSERT_FALSE(api::constantTimeEquals("abc", "abcd", 49));
    TEST_ASSERT_FALSE(api::constantTimeEquals(nullptr, "x", 49));
}

void test_extract_bearer_token() {
    char out[49] = {0};
    TEST_ASSERT_TRUE(api::extractBearerToken("Bearer mytoken123", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("mytoken123", out);
}

void test_extract_bearer_rejects_non_bearer() {
    char out[49] = {0};
    TEST_ASSERT_FALSE(api::extractBearerToken("Basic abc", out, sizeof(out)));
    TEST_ASSERT_FALSE(api::extractBearerToken("Bearer ", out, sizeof(out)));
    TEST_ASSERT_FALSE(api::extractBearerToken(nullptr, out, sizeof(out)));
}

void test_extract_bearer_rejects_overflow() {
    char out[8] = {0};  // too small for the token
    TEST_ASSERT_FALSE(api::extractBearerToken("Bearer abcdefghijklmnop", out, sizeof(out)));
}

void test_valid_window() {
    schedule::TimeWindow ok(630, 870);
    schedule::TimeWindow cross(1320, 120);  // crosses midnight, still valid
    schedule::TimeWindow bad(630, 2000);    // end out of range
    TEST_ASSERT_TRUE(api::isValidWindow(ok));
    TEST_ASSERT_TRUE(api::isValidWindow(cross));
    TEST_ASSERT_FALSE(api::isValidWindow(bad));
}

void test_valid_schedule_rejects_too_many_windows() {
    schedule::ChannelSchedule s;
    s.windowCount = schedule::kMaxWindowsPerChannel + 1;
    TEST_ASSERT_FALSE(api::isValidSchedule(s));
}

void test_valid_thresholds() {
    alert::TempThresholds ok;  // 20/31/0.5
    TEST_ASSERT_TRUE(api::isValidThresholds(ok));

    alert::TempThresholds inverted;
    inverted.lowC = 32.0f;
    inverted.highC = 31.0f;
    TEST_ASSERT_FALSE(api::isValidThresholds(inverted));

    alert::TempThresholds insane;
    insane.lowC = -50.0f;
    insane.highC = 99.0f;
    TEST_ASSERT_FALSE(api::isValidThresholds(insane));
}

void test_valid_channel_name() {
    TEST_ASSERT_TRUE(api::isValidChannelName("Heater"));
    TEST_ASSERT_FALSE(api::isValidChannelName(""));
    TEST_ASSERT_FALSE(api::isValidChannelName(nullptr));
    char tooLong[channel::kChannelNameSize + 4];
    memset(tooLong, 'A', sizeof(tooLong) - 1);
    tooLong[sizeof(tooLong) - 1] = '\0';
    TEST_ASSERT_FALSE(api::isValidChannelName(tooLong));
    TEST_ASSERT_FALSE(api::isValidChannelName("Bad\x01Ctrl"));
}

void test_valid_tz() {
    TEST_ASSERT_TRUE(api::isValidTz("UTC0"));
    TEST_ASSERT_TRUE(api::isValidTz("IST-5:30"));
    TEST_ASSERT_FALSE(api::isValidTz(""));
}

void test_valid_channel_index() {
    TEST_ASSERT_TRUE(api::isValidChannelIndex(0, 4));
    TEST_ASSERT_TRUE(api::isValidChannelIndex(3, 4));
    TEST_ASSERT_FALSE(api::isValidChannelIndex(4, 4));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_constant_time_equal_match);
    RUN_TEST(test_constant_time_equal_mismatch);
    RUN_TEST(test_extract_bearer_token);
    RUN_TEST(test_extract_bearer_rejects_non_bearer);
    RUN_TEST(test_extract_bearer_rejects_overflow);
    RUN_TEST(test_valid_window);
    RUN_TEST(test_valid_schedule_rejects_too_many_windows);
    RUN_TEST(test_valid_thresholds);
    RUN_TEST(test_valid_channel_name);
    RUN_TEST(test_valid_tz);
    RUN_TEST(test_valid_channel_index);
    return UNITY_END();
}
