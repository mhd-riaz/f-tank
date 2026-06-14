#ifndef API_API_VALIDATION_H
#define API_API_VALIDATION_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "alert/AlertTypes.h"
#include "schedule/Schedule.h"
#include "storage/PersistentConfig.h"

/**
 * @file ApiValidation.h
 * @brief Pure validators for all externally-supplied config (NFR-8).
 *
 * Every value accepted from the app/cloud/BLE MUST pass these bounds checks
 * before being applied or persisted. Hardware-free -> unit-tested natively.
 */
namespace api {

/// Validate a single schedule window (minutes-of-day in range; cross-midnight ok).
inline bool isValidWindow(const schedule::TimeWindow& w) {
    return w.startMin <= schedule::kMaxMinuteOfDay && w.endMin <= schedule::kMaxMinuteOfDay;
}

/// Validate a whole channel schedule (count + every active window).
inline bool isValidSchedule(const schedule::ChannelSchedule& s) {
    if (s.windowCount > schedule::kMaxWindowsPerChannel) {
        return false;
    }
    for (uint8_t i = 0; i < s.windowCount; ++i) {
        if (!api::isValidWindow(s.windows[i])) {
            return false;
        }
    }
    return true;
}

/// Validate temperature thresholds: ordered, sane range, non-negative hysteresis.
inline bool isValidThresholds(const alert::TempThresholds& t) {
    if (!(t.lowC < t.highC)) {
        return false;  // low must be strictly below high
    }
    if (t.lowC < -10.0f || t.highC > 60.0f) {
        return false;  // outside any plausible aquarium range
    }
    if (t.hysteresisC < 0.0f || t.hysteresisC > 5.0f) {
        return false;
    }
    return true;
}

/// Validate a channel display name: non-empty, fits, printable ASCII (NFR-9).
inline bool isValidChannelName(const char* name) {
    if (name == nullptr) {
        return false;
    }
    const size_t len = strlen(name);
    if (len == 0 || len >= channel::kChannelNameSize) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        if (c < 0x20 || c > 0x7E) {
            return false;  // control or non-ASCII byte
        }
    }
    return true;
}

/// Validate a POSIX TZ string: non-empty, fits, printable ASCII.
inline bool isValidTz(const char* tz) {
    if (tz == nullptr) {
        return false;
    }
    const size_t len = strlen(tz);
    if (len == 0 || len >= storage::kTzStringSize) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(tz[i]);
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

/// Validate a channel index against the active count.
inline bool isValidChannelIndex(uint8_t index, uint8_t channelCount) {
    return index < channelCount;
}

/// Scope of a factory-reset request (FR-10).
enum class ResetScope : uint8_t {
    kNone = 0,        ///< unrecognized / not requested
    kForgetWifi = 1,  ///< clear WiFi creds + device token only (re-provision)
    kFactory = 2,     ///< full wipe: creds, token, config defaults, logs, cloud
};

/// Parse a reset-scope string from the API. Unknown values map to kNone so the
/// caller rejects them (never silently performs an unintended wipe, NFR-8).
inline ResetScope parseResetScope(const char* s) {
    if (s == nullptr) {
        return ResetScope::kNone;
    }
    if (strcmp(s, "wifi") == 0) {
        return ResetScope::kForgetWifi;
    }
    if (strcmp(s, "factory") == 0) {
        return ResetScope::kFactory;
    }
    return ResetScope::kNone;
}

}  // namespace api

#endif  // API_API_VALIDATION_H
