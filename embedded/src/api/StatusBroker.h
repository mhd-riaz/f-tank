#ifndef API_STATUS_BROKER_H
#define API_STATUS_BROKER_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "channel/ChannelController.h"

/**
 * @file StatusBroker.h
 * @brief Thread-safe publish/read of live device status (AD-1).
 *
 * The control loop publishes a snapshot each tick; the networking task reads a
 * consistent copy for REST/WebSocket responses. Mutex-guarded so the two tasks
 * never see a half-updated snapshot.
 */
namespace api {

/// Immutable view of current device state for the app.
struct StatusSnapshot {
    float temperatureC = -127.0f;
    bool temperatureValid = false;
    uint16_t activeAlertMask = 0;
    uint8_t timeSource = 0;  ///< mirrors ftime::TimeSource
    uint8_t wifiState = 0;   ///< mirrors network::WifiState
    uint8_t channelCount = 0;
    bool channelEnergized[channel::kMaxChannels] = {false};
    uint16_t minuteOfDay = 0;
};

class StatusBroker {
  public:
    void begin();

    /// Publish the latest snapshot (control loop).
    void publish(const StatusSnapshot& snap);

    /// Read a consistent copy (networking task). Returns false if locking failed.
    bool read(StatusSnapshot& out);

  private:
    SemaphoreHandle_t mutex_ = nullptr;
    StatusSnapshot snap_;
};

}  // namespace api

#endif  // API_STATUS_BROKER_H
