#ifndef LOG_EVENT_LOGGER_H
#define LOG_EVENT_LOGGER_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config/features.h"
#include "log/LogTypes.h"
#include "time/TimeService.h"

#if FT_FEATURE_SD_LOG
#include "log/LogStore.h"
#endif

/**
 * @file EventLogger.h
 * @brief On-demand event logger shared by the control loop and the API task.
 *
 * Capture is gated by a recording flag the app toggles over the local API
 * (FR-29: record/stream only on request — no continuous flood). When recording,
 * typed events are appended to a bounded RAM ring (on-demand snapshot, both
 * SKUs) and, on the 8 MB Premium SKU, persisted to LittleFS (FR-30).
 *
 * Mutex-guarded like StatusBroker/ConfigBroker: log* methods run on the control
 * loop; setRecording()/snapshot()/clear() run on the networking task. The single
 * mutex is the only shared mutable state, so the two contexts never race
 * (NFR-1). Persistent flash writes happen only from the control loop.
 */
namespace logging {

class EventLogger {
  public:
    explicit EventLogger(ftime::TimeService& time) : time_(time) {}

    /// Create the mutex and (8 MB SKU) mount the persistent store. Call once at boot.
    void begin();

    // ---- Networking task ----

    /// Enable/disable on-demand capture (app request).
    void setRecording(bool on);
    bool recording() const;

    /// Copy up to `max` most-recent RAM records, oldest-first, into out.
    /// Reports the current recording flag and persisted record count.
    /// Returns the number of records copied.
    uint8_t snapshot(LogRecord* out, uint8_t max, bool& recordingOut, uint16_t& persistedOut) const;

    /// Clear the RAM ring (and persistent store on the 8 MB SKU).
    bool clear();

    // ---- Control loop (no-op unless recording) ----

    void logBoot(uint8_t reason);
    void logChannel(uint8_t index, bool on);
    void logTemperature(float celsius);
    void logAlert(uint8_t alertId, bool set, uint16_t activeMask);
    void logTimeSource(uint8_t source);
    void logConfigApplied();

  private:
    void record(LogEventType type, uint8_t arg, int16_t centiCelsius, uint16_t detail);

    ftime::TimeService& time_;
    SemaphoreHandle_t mutex_ = nullptr;
    LogRingBuffer ring_;
    bool recording_ = false;
#if FT_FEATURE_SD_LOG
    LogStore store_;
    bool storeReady_ = false;
#endif
};

}  // namespace logging

#endif  // LOG_EVENT_LOGGER_H
