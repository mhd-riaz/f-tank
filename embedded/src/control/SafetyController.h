#ifndef CONTROL_SAFETY_CONTROLLER_H
#define CONTROL_SAFETY_CONTROLLER_H

#include "alert/AlertManager.h"
#include "alert/AlertTypes.h"
#include "channel/ChannelController.h"
#include "control/ScheduleRunner.h"
#include "sensor/TemperatureSensor.h"

/**
 * @file SafetyController.h
 * @brief Maps temperature state to alerts + heater-cut safety action.
 *
 * On high temp or sensor fault: raises the alert and force-offs every channel
 * flagged cutOnOverTemp (FR-23/24). Recovers with hysteresis when the band is
 * safe again (FR-25). Pure-ish glue; non-blocking.
 */
namespace control {

class SafetyController {
  public:
    SafetyController(sensor::TemperatureSensor& temp, alert::AlertManager& alerts,
                     channel::ChannelController& channels, ScheduleRunner& runner)
        : temp_(temp), alerts_(alerts), channels_(channels), runner_(runner) {}

    /// Precompute which channels are heater-cut targets. Call after channels.begin().
    void begin();

    void setThresholds(const alert::TempThresholds& th) {
        thresholds_ = th;
    }
    const alert::TempThresholds& thresholds() const {
        return thresholds_;
    }

    /// Cooperative servicing; non-blocking. Call frequently from loop().
    void update();

  private:
    void applyCut(bool cut);

    sensor::TemperatureSensor& temp_;
    alert::AlertManager& alerts_;
    channel::ChannelController& channels_;
    ScheduleRunner& runner_;

    alert::TempThresholds thresholds_;
    uint16_t cutTargetMask_ = 0;
    bool highActive_ = false;
    bool lowActive_ = false;
    bool cutEngaged_ = false;
};

}  // namespace control

#endif  // CONTROL_SAFETY_CONTROLLER_H
