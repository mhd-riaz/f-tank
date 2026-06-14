#ifndef SCHEDULE_SCHEDULE_H
#define SCHEDULE_SCHEDULE_H

#include <cstdint>

/**
 * @file Schedule.h
 * @brief Pure per-channel schedule model + evaluator (hardware-free).
 *
 * Native-testable (header-only). A channel is ON when the evaluator says so;
 * mapping to a GPIO level happens later via RelayPolarity (NFR-11). Windows use
 * minutes-since-midnight [0,1439] and use half-open [start,end) semantics so
 * adjacent windows never double-count. A window may cross midnight (FR-14).
 */
namespace schedule {

/// Max windows a single channel's daily schedule may hold (FR-14).
constexpr uint8_t kMaxWindowsPerChannel = 8;

/// Largest valid minute-of-day value.
constexpr uint16_t kMaxMinuteOfDay = 1439;

/// One time range within a channel schedule.
struct TimeWindow {
    uint16_t startMin;  ///< inclusive, minutes since midnight [0,1439]
    uint16_t endMin;    ///< exclusive, minutes since midnight [0,1439]

    // Explicit constructors: keeps brace-init working under C++11 (ESP32 core),
    // where default member initializers would disqualify aggregate init.
    TimeWindow() : startMin(0), endMin(0) {}
    TimeWindow(uint16_t start, uint16_t end) : startMin(start), endMin(end) {}
};

/// A single channel's schedule.
struct ChannelSchedule {
    TimeWindow windows[kMaxWindowsPerChannel];
    uint8_t windowCount = 0;  ///< number of active entries in `windows`
    bool inverted = false;    ///< false: ON during windows; true: ON except windows
};

/// True if `minute` falls within a single window (half-open; wraps midnight).
inline bool isWindowActive(const TimeWindow& w, uint16_t minute) {
    if (w.startMin == w.endMin) {
        return false;  // zero-length window is never active
    }
    if (w.startMin < w.endMin) {
        return minute >= w.startMin && minute < w.endMin;
    }
    // Cross-midnight: active from start..midnight and midnight..end.
    return minute >= w.startMin || minute < w.endMin;
}

/// True if a window has in-range, non-trivial bounds.
inline bool isValidWindow(const TimeWindow& w) {
    return w.startMin <= kMaxMinuteOfDay && w.endMin <= kMaxMinuteOfDay;
}

/// Resolve whether a channel should be energized at `minute` of the day.
inline bool isEnergizedAt(const ChannelSchedule& s, uint16_t minute) {
    bool anyActive = false;
    const uint8_t count =
        s.windowCount <= kMaxWindowsPerChannel ? s.windowCount : kMaxWindowsPerChannel;
    for (uint8_t i = 0; i < count; ++i) {
        if (isWindowActive(s.windows[i], minute)) {
            anyActive = true;
            break;
        }
    }
    return s.inverted ? !anyActive : anyActive;
}

}  // namespace schedule

#endif  // SCHEDULE_SCHEDULE_H
