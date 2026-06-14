#include <unity.h>

#include "schedule/Schedule.h"
#include "schedule/Scheduler.h"

using schedule::ChannelSchedule;
using schedule::isEnergizedAt;
using schedule::isValidWindow;
using schedule::isWindowActive;
using schedule::kMaxMinuteOfDay;
using schedule::Scheduler;
using schedule::TimeWindow;

namespace {

void test_window_half_open() {
    TimeWindow w{630, 870};                    // 10:30-14:30
    TEST_ASSERT_TRUE(isWindowActive(w, 630));  // inclusive start
    TEST_ASSERT_TRUE(isWindowActive(w, 800));
    TEST_ASSERT_FALSE(isWindowActive(w, 870));  // exclusive end
    TEST_ASSERT_FALSE(isWindowActive(w, 629));
}

void test_zero_length_window_inactive() {
    TimeWindow w{600, 600};
    TEST_ASSERT_FALSE(isWindowActive(w, 600));
}

void test_cross_midnight_window() {
    TimeWindow w{1320, 120};                    // 22:00 -> 02:00
    TEST_ASSERT_TRUE(isWindowActive(w, 1320));  // 22:00
    TEST_ASSERT_TRUE(isWindowActive(w, 1439));  // 23:59
    TEST_ASSERT_TRUE(isWindowActive(w, 0));     // 00:00
    TEST_ASSERT_TRUE(isWindowActive(w, 119));   // 01:59
    TEST_ASSERT_FALSE(isWindowActive(w, 120));  // 02:00 exclusive
    TEST_ASSERT_FALSE(isWindowActive(w, 800));  // midday
}

void test_normal_multiwindow_light() {
    ChannelSchedule s;  // Light: 10:30-14:30, 17:00-20:00
    s.windows[0] = {630, 870};
    s.windows[1] = {1020, 1200};
    s.windowCount = 2;
    s.inverted = false;
    TEST_ASSERT_TRUE(isEnergizedAt(s, 700));    // in first
    TEST_ASSERT_FALSE(isEnergizedAt(s, 900));   // gap
    TEST_ASSERT_TRUE(isEnergizedAt(s, 1100));   // in second
    TEST_ASSERT_FALSE(isEnergizedAt(s, 1300));  // after
}

void test_inverted_pump() {
    ChannelSchedule s;  // Pump ON except 9:30-10:30, 14:00-15:00
    s.windows[0] = {570, 630};
    s.windows[1] = {840, 900};
    s.windowCount = 2;
    s.inverted = true;
    TEST_ASSERT_TRUE(isEnergizedAt(s, 0));     // midnight ON
    TEST_ASSERT_FALSE(isEnergizedAt(s, 600));  // in off-window 1
    TEST_ASSERT_TRUE(isEnergizedAt(s, 700));   // back ON
    TEST_ASSERT_FALSE(isEnergizedAt(s, 870));  // in off-window 2
    TEST_ASSERT_TRUE(isEnergizedAt(s, 1200));  // ON
}

void test_inverted_empty_is_always_on() {
    ChannelSchedule s;
    s.windowCount = 0;
    s.inverted = true;
    TEST_ASSERT_TRUE(isEnergizedAt(s, 0));
    TEST_ASSERT_TRUE(isEnergizedAt(s, 720));
}

void test_valid_window_bounds() {
    TEST_ASSERT_TRUE(isValidWindow(TimeWindow{0, 0}));                              // in range
    TEST_ASSERT_TRUE(isValidWindow(TimeWindow{kMaxMinuteOfDay, kMaxMinuteOfDay}));  // max edge
    TEST_ASSERT_TRUE(isValidWindow(TimeWindow{0, kMaxMinuteOfDay}));
    TEST_ASSERT_FALSE(isValidWindow(TimeWindow{0, 1440}));     // end out of range
    TEST_ASSERT_FALSE(isValidWindow(TimeWindow{1440, 0}));     // start out of range
    TEST_ASSERT_FALSE(isValidWindow(TimeWindow{2000, 3000}));  // both out of range
}

void test_energized_clamps_excess_window_count() {
    // windowCount above the array bound must be clamped, not over-read.
    ChannelSchedule s;
    s.windows[0] = {630, 870};
    s.windowCount = 200;  // absurd; evaluator clamps to kMaxWindowsPerChannel
    s.inverted = false;
    TEST_ASSERT_TRUE(isEnergizedAt(s, 700));   // first window still honored
    TEST_ASSERT_FALSE(isEnergizedAt(s, 900));  // outside any real window
}

void test_scheduler_channel_count_clamped() {
    Scheduler sch;
    sch.setChannelCount(250);  // far above kMaxChannels
    TEST_ASSERT_TRUE(sch.channelCount() <= channel::kMaxChannels);
}

void test_scheduler_desired_state_out_of_range_is_off() {
    Scheduler sch;
    sch.setChannelCount(2);
    sch.schedule(0).windowCount = 0;
    sch.schedule(0).inverted = true;  // ch0 always ON
    // In-range index respects schedule; out-of-range index must be OFF (guard).
    TEST_ASSERT_TRUE(sch.desiredState(0, 720));
    TEST_ASSERT_FALSE(sch.desiredState(2, 720));    // == count
    TEST_ASSERT_FALSE(sch.desiredState(200, 720));  // far out of range
}

void test_scheduler_compute_all() {
    Scheduler sch;
    sch.setChannelCount(2);
    sch.schedule(0).windows[0] = {630, 870};  // ch0 ON 10:30-14:30
    sch.schedule(0).windowCount = 1;
    sch.schedule(1).windowCount = 0;
    sch.schedule(1).inverted = true;  // ch1 always ON

    bool out[2] = {false, false};
    sch.computeAll(700, out);
    TEST_ASSERT_TRUE(out[0]);
    TEST_ASSERT_TRUE(out[1]);

    sch.computeAll(900, out);
    TEST_ASSERT_FALSE(out[0]);
    TEST_ASSERT_TRUE(out[1]);
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_window_half_open);
    RUN_TEST(test_zero_length_window_inactive);
    RUN_TEST(test_cross_midnight_window);
    RUN_TEST(test_normal_multiwindow_light);
    RUN_TEST(test_inverted_pump);
    RUN_TEST(test_inverted_empty_is_always_on);
    RUN_TEST(test_valid_window_bounds);
    RUN_TEST(test_energized_clamps_excess_window_count);
    RUN_TEST(test_scheduler_channel_count_clamped);
    RUN_TEST(test_scheduler_desired_state_out_of_range_is_off);
    RUN_TEST(test_scheduler_compute_all);
    return UNITY_END();
}
