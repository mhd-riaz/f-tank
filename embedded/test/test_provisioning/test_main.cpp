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

void test_oversized_password_rejected() {
    ProvisioningSession s;
    s.reset();
    s.setSsid("Net");
    char big[80];
    for (int i = 0; i < 79; ++i)
        big[i] = 'p';
    big[79] = '\0';  // exceeds kPasswordSize (64)
    TEST_ASSERT_FALSE(s.setPassword(big));
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kInvalid), static_cast<int>(s.state()));
    TEST_ASSERT_FALSE(s.isReady());
}

void test_invalid_field_after_ready_drops_out() {
    // Becoming ready then receiving a bad field must drop back out of ready
    // (a half-valid session must never be committed).
    ProvisioningSession s;
    s.reset();
    s.setSsid("Net");
    s.setPassword("pass");
    TEST_ASSERT_TRUE(s.isReady());
    TEST_ASSERT_FALSE(s.setSsid(""));  // invalid SSID overwrite
    TEST_ASSERT_FALSE(s.isReady());
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kInvalid), static_cast<int>(s.state()));
}

void test_reset_recovers_after_invalid() {
    ProvisioningSession s;
    s.reset();
    TEST_ASSERT_FALSE(s.setSsid(""));  // -> kInvalid
    s.reset();                         // start over
    TEST_ASSERT_EQUAL(static_cast<int>(SessionState::kIdle), static_cast<int>(s.state()));
    TEST_ASSERT_TRUE(s.setSsid("Net2"));
    TEST_ASSERT_TRUE(s.setPassword("pw"));
    TEST_ASSERT_TRUE(s.isReady());
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
    RUN_TEST(test_oversized_password_rejected);
    RUN_TEST(test_invalid_field_after_ready_drops_out);
    RUN_TEST(test_reset_recovers_after_invalid);
    RUN_TEST(test_invalid_tz_rejected);
    RUN_TEST(test_valid_tz_keeps_ready);
    return UNITY_END();
}
