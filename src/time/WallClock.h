#ifndef TIME_WALLCLOCK_H
#define TIME_WALLCLOCK_H

#include <cstdint>

/**
 * @file WallClock.h
 * @brief Hardware-free wall-clock type and pure time math.
 *
 * Deliberately free of Arduino/RTClib so it compiles and unit-tests under the
 * PlatformIO `native` environment (code-quality: keep pure logic testable).
 * Uses Howard Hinnant's well-known civil<->days algorithms for calendar math.
 */
namespace ftime {

/// Broken-down local wall-clock time. No timezone — device runs in local time.
struct WallClock {
    uint16_t year = 1970;
    uint8_t month = 1;   ///< 1-12
    uint8_t day = 1;     ///< 1-31
    uint8_t hour = 0;    ///< 0-23
    uint8_t minute = 0;  ///< 0-59
    uint8_t second = 0;  ///< 0-59
};

/// Minutes since local midnight — the unit the scheduler consumes (0-1439).
constexpr uint16_t minutesSinceMidnight(const WallClock& wc) {
    return static_cast<uint16_t>(wc.hour) * 60u + wc.minute;
}

/// Days since 1970-01-01 for a civil (proleptic Gregorian) date.
inline int32_t daysFromCivil(int32_t y, uint32_t m, uint32_t d) {
    y -= (m <= 2);
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = static_cast<uint32_t>(y - era * 400);             // [0, 399]
    const uint32_t doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
    const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            // [0, 146096]
    return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

/// Convert a wall-clock time to Unix epoch seconds (assumes year >= 1970).
inline uint32_t toUnixEpoch(const WallClock& wc) {
    const int32_t days = daysFromCivil(wc.year, wc.month, wc.day);
    return static_cast<uint32_t>(days) * 86400u + static_cast<uint32_t>(wc.hour) * 3600u +
           static_cast<uint32_t>(wc.minute) * 60u + wc.second;
}

/// Convert Unix epoch seconds back to a broken-down wall-clock time.
inline WallClock fromUnixEpoch(uint32_t epoch) {
    const int32_t days = static_cast<int32_t>(epoch / 86400u);
    const uint32_t rem = epoch % 86400u;

    int32_t z = days + 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = static_cast<uint32_t>(z - era * 146097);                // [0, 146096]
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);                // [0, 365]
    const uint32_t mp = (5 * doy + 2) / 153;                                     // [0, 11]
    const uint32_t d = doy - (153 * mp + 2) / 5 + 1;                             // [1, 31]
    const uint32_t m = mp < 10 ? mp + 3 : mp - 9;                                // [1, 12]
    const int32_t y = static_cast<int32_t>(yoe) + era * 400 + (m <= 2);

    WallClock wc;
    wc.year = static_cast<uint16_t>(y);
    wc.month = static_cast<uint8_t>(m);
    wc.day = static_cast<uint8_t>(d);
    wc.hour = static_cast<uint8_t>(rem / 3600u);
    wc.minute = static_cast<uint8_t>((rem % 3600u) / 60u);
    wc.second = static_cast<uint8_t>(rem % 60u);
    return wc;
}

}  // namespace ftime

#endif  // TIME_WALLCLOCK_H
