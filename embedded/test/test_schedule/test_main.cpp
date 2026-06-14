#include <unity.h>

#include "schedule/Schedule.h"
#include "schedule/Scheduler.h"

using schedule::ChannelSchedule;
using schedule::isEnergizedAt;
using schedule::isWindowActive;
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
    RUN_TEST(test_scheduler_compute_all);
    return UNITY_END();
}
