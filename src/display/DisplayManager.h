#ifndef DISPLAY_DISPLAY_MANAGER_H
#define DISPLAY_DISPLAY_MANAGER_H

#include <U8g2lib.h>

#include "alert/AlertManager.h"
#include "channel/ChannelController.h"
#include "sensor/TemperatureSensor.h"
#include "time/TimeService.h"

/**
 * @file DisplayManager.h
 * @brief SSD1306 128x32 OLED with an even, non-blocking screen rotation.
 *
 * Replaces the legacy uneven second()%10 split with a fixed rotation of four
 * screens (time/date, channels, temperature, status) shown for equal periods
 * (FR-27). Status screen surfaces active alerts and the time source (FR-28).
 */
namespace display {

/// Milliseconds each screen is shown before rotating.
constexpr uint32_t kScreenDurationMs = 3000;

/// Number of rotating screens.
constexpr uint8_t kScreenCount = 4;

class DisplayManager {
  public:
    DisplayManager(ftime::TimeService& time, sensor::TemperatureSensor& temp,
                   channel::ChannelController& channels, alert::AlertManager& alerts)
        : time_(time), temp_(temp), channels_(channels), alerts_(alerts) {}

    void begin();

    /// Cooperative servicing; non-blocking. Renders on a timer.
    void update(uint32_t nowMs);

  private:
    void renderTimeDate();
    void renderChannels();
    void renderTemperature();
    void renderStatus();

    U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2_{U8G2_R2, U8X8_PIN_NONE};
    ftime::TimeService& time_;
    sensor::TemperatureSensor& temp_;
    channel::ChannelController& channels_;
    alert::AlertManager& alerts_;

    uint8_t screen_ = 0;
    uint32_t lastRotateMs_ = 0;
    bool initialized_ = false;
};

}  // namespace display

#endif  // DISPLAY_DISPLAY_MANAGER_H
