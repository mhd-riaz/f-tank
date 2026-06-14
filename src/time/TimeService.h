#ifndef TIME_TIME_SERVICE_H
#define TIME_TIME_SERVICE_H

#include <RTClib.h>

#include "time/WallClock.h"

/**
 * @file TimeService.h
 * @brief Authoritative local time: DS3231 RTC with a millis() soft-clock fallback.
 *
 * Authority order (FR-5/6/7): NTP (M2) -> DS3231 RTC (offline authority) ->
 * millis() soft clock seeded from firmware build time. Never traps the loop.
 */
namespace ftime {

/// Which source currently backs the clock (for display/diagnostics, FR-28).
enum class TimeSource : uint8_t { kNtp, kRtc, kSoftClock };

/// Typed init result — failures are never silent (code-quality rule).
enum class TimeError : uint8_t { kOk, kRtcUnavailable, kRtcLostPower };

class TimeService {
  public:
    /**
     * @brief Initialize the RTC; fall back to the soft clock on failure.
     * @return kOk if the DS3231 is present and running; otherwise a typed
     *         error while the service degrades to the soft clock (still usable).
     */
    TimeError begin();

    /// Cooperative servicing; cheap and non-blocking (NFR-1). Call from loop().
    void update();

    /// Current local wall-clock time from the best available source.
    WallClock now() const;

    /// Minutes since local midnight (0-1439) — the scheduler's input.
    uint16_t minutesSinceMidnight() const;

    /// Last/active time authority, for status display.
    TimeSource source() const {
        return source_;
    }

    /// True when the active source is real (RTC/NTP), not the soft fallback.
    bool hasTrustedTime() const {
        return source_ != TimeSource::kSoftClock;
    }

    /**
     * @brief Push an externally-synced time (e.g. NTP) into the service.
     *        Updates the RTC when present and marks the source as NTP (FR-6).
     */
    void setFromExternal(const WallClock& wc);

  private:
    WallClock softClockNow() const;
    void seedSoftClock(uint32_t epochSeconds);

    mutable RTC_DS3231 rtc_;  ///< mutable: now() reads I2C from const methods
    TimeSource source_ = TimeSource::kSoftClock;
    bool rtcPresent_ = false;
    uint32_t softEpochBase_ = 0;   ///< epoch seconds at softMillisBase_
    uint32_t softMillisBase_ = 0;  ///< millis() captured when soft clock seeded
};

}  // namespace ftime

#endif  // TIME_TIME_SERVICE_H
