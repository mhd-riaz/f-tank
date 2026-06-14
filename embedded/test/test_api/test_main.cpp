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

void test_parse_reset_scope() {
    TEST_ASSERT_TRUE(api::parseResetScope("wifi") == api::ResetScope::kForgetWifi);
    TEST_ASSERT_TRUE(api::parseResetScope("factory") == api::ResetScope::kFactory);
}

void test_parse_reset_scope_rejects_unknown() {
    TEST_ASSERT_TRUE(api::parseResetScope("") == api::ResetScope::kNone);
    TEST_ASSERT_TRUE(api::parseResetScope("Factory") == api::ResetScope::kNone);  // case-sensitive
    TEST_ASSERT_TRUE(api::parseResetScope("all") == api::ResetScope::kNone);
    TEST_ASSERT_TRUE(api::parseResetScope(nullptr) == api::ResetScope::kNone);
}

void test_thresholds_boundaries() {
    // low must be strictly below high.
    alert::TempThresholds eq;
    eq.lowC = 25.0f;
    eq.highC = 25.0f;
    TEST_ASSERT_FALSE(api::isValidThresholds(eq));

    // Hysteresis range is [0, 5].
    alert::TempThresholds hNeg;
    hNeg.hysteresisC = -0.1f;
    TEST_ASSERT_FALSE(api::isValidThresholds(hNeg));
    alert::TempThresholds hMax;
    hMax.hysteresisC = 5.0f;
    TEST_ASSERT_TRUE(api::isValidThresholds(hMax));
    alert::TempThresholds hOver;
    hOver.hysteresisC = 5.1f;
    TEST_ASSERT_FALSE(api::isValidThresholds(hOver));
}

void test_thresholds_range_edges() {
    // low boundary -10 inclusive, high boundary 60 inclusive.
    alert::TempThresholds edge;
    edge.lowC = -10.0f;
    edge.highC = 60.0f;
    TEST_ASSERT_TRUE(api::isValidThresholds(edge));

    alert::TempThresholds lowUnder;
    lowUnder.lowC = -10.1f;
    lowUnder.highC = 40.0f;
    TEST_ASSERT_FALSE(api::isValidThresholds(lowUnder));

    alert::TempThresholds highOver;
    highOver.lowC = 20.0f;
    highOver.highC = 60.1f;
    TEST_ASSERT_FALSE(api::isValidThresholds(highOver));
}

void test_channel_name_length_edges() {
    // Longest accepted name = kChannelNameSize - 1 chars; one more is rejected.
    char maxName[channel::kChannelNameSize];
    memset(maxName, 'A', sizeof(maxName) - 1);
    maxName[sizeof(maxName) - 1] = '\0';
    TEST_ASSERT_TRUE(api::isValidChannelName(maxName));

    char overName[channel::kChannelNameSize + 1];
    memset(overName, 'A', sizeof(overName) - 1);
    overName[sizeof(overName) - 1] = '\0';
    TEST_ASSERT_FALSE(api::isValidChannelName(overName));
}

void test_channel_name_ascii_edges() {
    TEST_ASSERT_TRUE(api::isValidChannelName(" "));      // 0x20 lowest printable
    TEST_ASSERT_TRUE(api::isValidChannelName("~"));      // 0x7E highest printable
    TEST_ASSERT_FALSE(api::isValidChannelName("\x7F"));  // DEL just above printable
}

void test_tz_length_edge() {
    char maxTz[storage::kTzStringSize];
    memset(maxTz, 'X', sizeof(maxTz) - 1);
    maxTz[sizeof(maxTz) - 1] = '\0';
    TEST_ASSERT_TRUE(api::isValidTz(maxTz));  // exactly fits

    char overTz[storage::kTzStringSize + 1];
    memset(overTz, 'X', sizeof(overTz) - 1);
    overTz[sizeof(overTz) - 1] = '\0';
    TEST_ASSERT_FALSE(api::isValidTz(overTz));
}

void test_extract_bearer_skips_extra_spaces() {
    char out[49] = {0};
    TEST_ASSERT_TRUE(api::extractBearerToken("Bearer    spacedtoken", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("spacedtoken", out);
}

void test_extract_bearer_exact_fit_buffer() {
    // Buffer just large enough for "abc" + null.
    char out[4] = {0};
    TEST_ASSERT_TRUE(api::extractBearerToken("Bearer abc", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("abc", out);
    // One char longer than the buffer can hold -> rejected, no overflow.
    char out2[4] = {0};
    TEST_ASSERT_FALSE(api::extractBearerToken("Bearer abcd", out2, sizeof(out2)));
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
    RUN_TEST(test_parse_reset_scope);
    RUN_TEST(test_parse_reset_scope_rejects_unknown);
    RUN_TEST(test_thresholds_boundaries);
    RUN_TEST(test_thresholds_range_edges);
    RUN_TEST(test_channel_name_length_edges);
    RUN_TEST(test_channel_name_ascii_edges);
    RUN_TEST(test_tz_length_edge);
    RUN_TEST(test_extract_bearer_skips_extra_spaces);
    RUN_TEST(test_extract_bearer_exact_fit_buffer);
    return UNITY_END();
}
