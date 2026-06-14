#include <unity.h>

#include "provisioning/ProvisioningSession.h"

using provisioning::ProvisioningSession;
using provisioning::SessionState;

namespace {

void test_idle_after_reset() {
    ProvisioningSession s;
    s.reset();
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kIdle), static_cast<int>(s.state()));
    TEST_ASSERT_FALSE(s.isReady());
}

void test_collecting_then_ready() {
    ProvisioningSession s;
    s.reset();
    TEST_ASSERT_TRUE(s.setSsid("MyNetwork"));
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kCollecting), static_cast<int>(s.state()));
    TEST_ASSERT_TRUE(s.setPassword("secretpass"));
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kReady), static_cast<int>(s.state()));
    TEST_ASSERT_TRUE(s.isReady());
    TEST_ASSERT_EQUAL_STRING("MyNetwork", s.ssid());
    TEST_ASSERT_EQUAL_STRING("secretpass", s.password());
}

void test_open_network_empty_password_ok() {
    ProvisioningSession s;
    s.reset();
    s.setSsid("OpenNet");
    TEST_ASSERT_TRUE(s.setPassword(""));  // open network
    TEST_ASSERT_TRUE(s.isReady());
}

void test_empty_ssid_rejected() {
    ProvisioningSession s;
    s.reset();
    TEST_ASSERT_FALSE(s.setSsid(""));
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kInvalid), static_cast<int>(s.state()));
}

void test_oversized_ssid_rejected() {
    ProvisioningSession s;
    s.reset();
    char big[40];
    for (int i = 0; i < 39; ++i)
        big[i] = 'A';
    big[39] = '\0';
    TEST_ASSERT_FALSE(s.setSsid(big));
}

void test_invalid_tz_rejected() {
    ProvisioningSession s;
    s.reset();
    s.setSsid("Net");
    s.setPassword("pass");
    TEST_ASSERT_FALSE(s.setTz(""));
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kInvalid), static_cast<int>(s.state()));
}

void test_valid_tz_keeps_ready() {
    ProvisioningSession s;
    s.reset();
    s.setSsid("Net");
    s.setPassword("pass");
    TEST_ASSERT_TRUE(s.setTz("IST-5:30"));
    TEST_ASSERT_TRUE(s.isReady());
    TEST_ASSERT_EQUAL_STRING("IST-5:30", s.tz());
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_idle_after_reset);
    RUN_TEST(test_collecting_then_ready);
    RUN_TEST(test_open_network_empty_password_ok);
    RUN_TEST(test_empty_ssid_rejected);
    RUN_TEST(test_oversized_ssid_rejected);
    RUN_TEST(test_invalid_tz_rejected);
    RUN_TEST(test_valid_tz_keeps_ready);
    return UNITY_END();
}
