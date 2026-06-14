#include "time/TimeService.h"

#include <Arduino.h>
#include <Wire.h>

#include "config/pins.h"

namespace ftime {
namespace {

/// Firmware build time, used to seed the soft clock / a power-lost RTC.
WallClock buildTimeWallClock() {
    const DateTime build(F(__DATE__), F(__TIME__));
    WallClock wc;
    wc.year = build.year();
    wc.month = build.month();
    wc.day = build.day();
    wc.hour = build.hour();
    wc.minute = build.minute();
    wc.second = build.second();
    return wc;
}

WallClock fromDateTime(const DateTime& dt) {
    WallClock wc;
    wc.year = dt.year();
    wc.month = dt.month();
    wc.day = dt.day();
    wc.hour = dt.hour();
    wc.minute = dt.minute();
    wc.second = dt.second();
    return wc;
}

}  // namespace

TimeError TimeService::begin() {
    Wire.begin(pins::kI2cSda, pins::kI2cScl);

    if (!rtc_.begin(&Wire)) {
        rtcPresent_ = false;
        seedSoftClock(toUnixEpoch(buildTimeWallClock()));
        source_ = TimeSource::kSoftClock;
        return TimeError::kRtcUnavailable;
    }

    rtcPresent_ = true;
    if (rtc_.lostPower()) {
        // RTC alive but unreliable — seed it from build time so time-of-day is
        // at least plausible until NTP corrects it (FR-7).
        rtc_.adjust(DateTime(F(__DATE__), F(__TIME__)));
        source_ = TimeSource::kRtc;
        return TimeError::kRtcLostPower;
    }

    source_ = TimeSource::kRtc;
    return TimeError::kOk;
}

void TimeService::update() {
    // RTC/NTP times are read on demand; the soft clock derives from millis().
    // Nothing periodic required here yet — kept non-blocking by design.
}

WallClock TimeService::now() const {
    if (rtcPresent_ && source_ != TimeSource::kSoftClock) {
        return fromDateTime(rtc_.now());
    }
    return softClockNow();
}

uint16_t TimeService::minutesSinceMidnight() const {
    return ftime::minutesSinceMidnight(now());
}

void TimeService::setFromExternal(const WallClock& wc) {
    const uint32_t epoch = toUnixEpoch(wc);
    if (rtcPresent_) {
        rtc_.adjust(DateTime(epoch));
        source_ = TimeSource::kNtp;
    } else {
        seedSoftClock(epoch);
        source_ = TimeSource::kSoftClock;  // still soft-backed, but NTP-corrected
    }
}

WallClock TimeService::softClockNow() const {
    const uint32_t elapsedSec = (millis() - softMillisBase_) / 1000u;
    return fromUnixEpoch(softEpochBase_ + elapsedSec);
}

void TimeService::seedSoftClock(uint32_t epochSeconds) {
    softEpochBase_ = epochSeconds;
    softMillisBase_ = millis();
}

}  // namespace ftime
