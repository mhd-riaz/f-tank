#include <unity.h>

#include <cstring>

#include "cloud/CloudTopics.h"

using cloud::buildTopic;
using cloud::isValidDeviceId;
using cloud::kSuffixTelemetry;
using cloud::kTopicBufSize;

namespace {

void test_valid_device_id() {
    TEST_ASSERT_TRUE(isValidDeviceId("ftank-abc123"));
    TEST_ASSERT_TRUE(isValidDeviceId("Device_01"));
    TEST_ASSERT_FALSE(isValidDeviceId(""));
    TEST_ASSERT_FALSE(isValidDeviceId(nullptr));
}

void test_device_id_rejects_injection() {
    // MQTT wildcards / separators must never be accepted into a topic (NFR-8).
    TEST_ASSERT_FALSE(isValidDeviceId("dev/evil"));
    TEST_ASSERT_FALSE(isValidDeviceId("dev+wild"));
    TEST_ASSERT_FALSE(isValidDeviceId("dev#all"));
    TEST_ASSERT_FALSE(isValidDeviceId("dev ice"));
    TEST_ASSERT_FALSE(isValidDeviceId("dev\x01ctrl"));
}

void test_device_id_length_bound() {
    char tooLong[40];
    memset(tooLong, 'a', sizeof(tooLong) - 1);
    tooLong[sizeof(tooLong) - 1] = '\0';
    TEST_ASSERT_FALSE(isValidDeviceId(tooLong));
}

void test_build_topic() {
    char out[kTopicBufSize];
    TEST_ASSERT_TRUE(buildTopic("dev123", kSuffixTelemetry, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ftank/dev123/telemetry", out);
}

void test_build_topic_rejects_bad_id() {
    char out[kTopicBufSize];
    TEST_ASSERT_FALSE(buildTopic("dev/evil", kSuffixTelemetry, out, sizeof(out)));
    TEST_ASSERT_FALSE(buildTopic("", kSuffixTelemetry, out, sizeof(out)));
}

void test_build_topic_rejects_overflow() {
    char tiny[8];
    TEST_ASSERT_FALSE(buildTopic("dev123", kSuffixTelemetry, tiny, sizeof(tiny)));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_device_id);
    RUN_TEST(test_device_id_rejects_injection);
    RUN_TEST(test_device_id_length_bound);
    RUN_TEST(test_build_topic);
    RUN_TEST(test_build_topic_rejects_bad_id);
    RUN_TEST(test_build_topic_rejects_overflow);
    return UNITY_END();
}
