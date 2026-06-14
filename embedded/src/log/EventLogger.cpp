#include "log/EventLogger.h"

#include "time/WallClock.h"

namespace logging {

void EventLogger::begin() {
    if (mutex_ == nullptr) {
        mutex_ = xSemaphoreCreateMutex();
    }
#if FT_FEATURE_SD_LOG
    storeReady_ = store_.begin();
#endif
}

void EventLogger::setRecording(bool on) {
    if (mutex_ == nullptr) {
        return;
    }
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        recording_ = on;
        xSemaphoreGive(mutex_);
    }
}

bool EventLogger::recording() const {
    if (mutex_ == nullptr) {
        return false;
    }
    bool r = false;
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        r = recording_;
        xSemaphoreGive(mutex_);
    }
    return r;
}

uint8_t EventLogger::snapshot(LogRecord* out, uint8_t max, bool& recordingOut,
                              uint16_t& persistedOut) const {
    recordingOut = false;
    persistedOut = 0;
    if (mutex_ == nullptr || out == nullptr || max == 0) {
        return 0;
    }
    uint8_t produced = 0;
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        recordingOut = recording_;
        const uint8_t avail = ring_.size();
        const uint8_t n = avail < max ? avail : max;
        const uint8_t start = static_cast<uint8_t>(avail - n);  // oldest of the n newest
        for (uint8_t i = 0; i < n; ++i) {
            out[i] = ring_.at(static_cast<uint8_t>(start + i));
        }
        produced = n;
#if FT_FEATURE_SD_LOG
        persistedOut = storeReady_ ? store_.count() : 0;
#endif
        xSemaphoreGive(mutex_);
    }
    return produced;
}

bool EventLogger::clear() {
    if (mutex_ == nullptr) {
        return false;
    }
    bool ok = false;
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        ring_.clear();
        ok = true;
#if FT_FEATURE_SD_LOG
        if (storeReady_) {
            ok = store_.clear();
        }
#endif
        xSemaphoreGive(mutex_);
    }
    return ok;
}

void EventLogger::record(LogEventType type, uint8_t arg, int16_t centiCelsius, uint16_t detail) {
    if (mutex_ == nullptr) {
        return;
    }
    // Fast gate: skip all work (incl. the RTC read below) when not recording.
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    const bool rec = recording_;
    xSemaphoreGive(mutex_);
    if (!rec) {
        return;
    }

    const ftime::WallClock wc = time_.now();
    LogRecord r;
    r.epoch = ftime::toUnixEpoch(wc);
    r.minuteOfDay = ftime::minutesSinceMidnight(wc);
    r.type = static_cast<uint8_t>(type);
    r.arg = arg;
    r.centiCelsius = centiCelsius;
    r.detail = detail;

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    if (recording_) {  // re-check under lock; app may have stopped meanwhile
        ring_.push(r);
#if FT_FEATURE_SD_LOG
        if (storeReady_) {
            store_.append(r);
        }
#endif
    }
    xSemaphoreGive(mutex_);
}

void EventLogger::logBoot(uint8_t reason) {
    record(LogEventType::kBoot, reason, 0, 0);
}

void EventLogger::logChannel(uint8_t index, bool on) {
    record(on ? LogEventType::kChannelOn : LogEventType::kChannelOff, index, 0, 0);
}

void EventLogger::logTemperature(float celsius) {
    const float scaled = celsius * 100.0f + (celsius >= 0.0f ? 0.5f : -0.5f);
    record(LogEventType::kTemperature, 0, static_cast<int16_t>(scaled), 0);
}

void EventLogger::logAlert(uint8_t alertId, bool set, uint16_t activeMask) {
    record(set ? LogEventType::kAlertSet : LogEventType::kAlertClear, alertId, 0, activeMask);
}

void EventLogger::logTimeSource(uint8_t source) {
    record(LogEventType::kTimeSource, source, 0, 0);
}

void EventLogger::logConfigApplied() {
    record(LogEventType::kConfigApplied, 0, 0, 0);
}

}  // namespace logging
