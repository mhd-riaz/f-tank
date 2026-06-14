#include <unity.h>

#include "alert/AlertTypes.h"

using alert::AlertId;
using alert::evaluateBand;
using alert::Severity;
using alert::severityOf;
using alert::TempBand;
using alert::TempThresholds;

namespace {

TempThresholds thresholds() {
    TempThresholds th;  // low 20, high 31, hysteresis 0.5
    return th;
}

void test_band_safe_middle() {
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kSafe),
                      static_cast<int>(evaluateBand(25.0f, thresholds(), false, false)));
}

void test_band_high_trigger() {
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kHigh),
                      static_cast<int>(evaluateBand(31.0f, thresholds(), false, false)));
}

void test_band_low_trigger() {
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kLow),
                      static_cast<int>(evaluateBand(20.0f, thresholds(), false, false)));
}

void test_high_hysteresis_holds() {
    // High latched at 31; with hysteresis 0.5 it stays high until < 30.5.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kHigh),
                      static_cast<int>(evaluateBand(30.7f, thresholds(), true, false)));
    // Drops below 30.5 -> clears to safe.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kSafe),
                      static_cast<int>(evaluateBand(30.4f, thresholds(), true, false)));
}

void test_low_hysteresis_holds() {
    // Low latched at 20; stays low until > 20.5.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kLow),
                      static_cast<int>(evaluateBand(20.3f, thresholds(), false, true)));
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kSafe),
                      static_cast<int>(evaluateBand(20.6f, thresholds(), false, true)));
}

void test_no_flap_without_hysteresis_breach() {
    // Hovering just under high while latched should NOT clear.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kHigh),
                      static_cast<int>(evaluateBand(30.9f, thresholds(), true, false)));
}

void test_high_takes_precedence_when_both_latched() {
    // If both flags are latched, a high reading must resolve to kHigh (heater
    // cut is the safety-critical action and is evaluated first).
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kHigh),
                      static_cast<int>(evaluateBand(31.0f, thresholds(), true, true)));
    // A safe-middle reading with both latched clears to safe.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kSafe),
                      static_cast<int>(evaluateBand(25.0f, thresholds(), true, true)));
}

void test_exact_high_hysteresis_boundary_holds() {
    // Latched high clears only BELOW high - hysteresis (30.5). Exactly 30.5 holds.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kHigh),
                      static_cast<int>(evaluateBand(30.5f, thresholds(), true, false)));
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kSafe),
                      static_cast<int>(evaluateBand(30.49f, thresholds(), true, false)));
}

void test_exact_low_hysteresis_boundary_holds() {
    // Latched low clears only ABOVE low + hysteresis (20.5). Exactly 20.5 holds.
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kLow),
                      static_cast<int>(evaluateBand(20.5f, thresholds(), false, true)));
    TEST_ASSERT_EQUAL(static_cast<int>(TempBand::kSafe),
                      static_cast<int>(evaluateBand(20.51f, thresholds(), false, true)));
}

void test_severity_mapping() {
    TEST_ASSERT_EQUAL(static_cast<int>(Severity::kCritical),
                      static_cast<int>(severityOf(AlertId::kHighTemp)));
    TEST_ASSERT_EQUAL(static_cast<int>(Severity::kCritical),
                      static_cast<int>(severityOf(AlertId::kSensorFault)));
    TEST_ASSERT_EQUAL(static_cast<int>(Severity::kWarning),
                      static_cast<int>(severityOf(AlertId::kLowTemp)));
    TEST_ASSERT_EQUAL(static_cast<int>(Severity::kInfo),
                      static_cast<int>(severityOf(AlertId::kNtpFailure)));
}

void test_severity_rtc_is_warning() {
    TEST_ASSERT_EQUAL(static_cast<int>(Severity::kWarning),
                      static_cast<int>(severityOf(AlertId::kRtcFailure)));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_band_safe_middle);
    RUN_TEST(test_band_high_trigger);
    RUN_TEST(test_band_low_trigger);
    RUN_TEST(test_high_hysteresis_holds);
    RUN_TEST(test_low_hysteresis_holds);
    RUN_TEST(test_no_flap_without_hysteresis_breach);
    RUN_TEST(test_high_takes_precedence_when_both_latched);
    RUN_TEST(test_exact_high_hysteresis_boundary_holds);
    RUN_TEST(test_exact_low_hysteresis_boundary_holds);
    RUN_TEST(test_severity_mapping);
    RUN_TEST(test_severity_rtc_is_warning);
    return UNITY_END();
}
