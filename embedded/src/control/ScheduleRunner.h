#ifndef CONTROL_SCHEDULE_RUNNER_H
#define CONTROL_SCHEDULE_RUNNER_H

#include "channel/ChannelController.h"
#include "schedule/Scheduler.h"

/**
 * @file ScheduleRunner.h
 * @brief Bridges computed schedule states to relay hardware, non-blocking.
 *
 * Applies at most one relay transition per stagger interval so multiple
 * channels never switch in the same instant (load distribution, FR-16). The
 * boot path computes states first and drives them in one shot for zero flicker
 * (FR-13); ongoing changes are staggered.
 */
namespace control {

/// Minimum spacing between successive relay transitions (inrush limiting).
constexpr uint32_t kStaggerIntervalMs = 40;

class ScheduleRunner {
  public:
    ScheduleRunner(schedule::Scheduler& scheduler, channel::ChannelController& channels)
        : scheduler_(scheduler), channels_(channels) {}

    /**
     * @brief Service the runner; call frequently from loop(). Non-blocking.
     * @param minuteOfDay current local minute-of-day [0,1439]
     * @param nowMs       millis() timestamp
     */
    void update(uint16_t minuteOfDay, uint32_t nowMs);

    /// Force a channel OFF regardless of schedule (safety override, FR-23).
    /// Cleared channels resume normal scheduled behavior.
    void setForceOff(uint8_t index, bool forceOff);

  private:
    schedule::Scheduler& scheduler_;
    channel::ChannelController& channels_;
    uint16_t forceOffMask_ = 0;
    uint32_t lastStaggerMs_ = 0;
};

}  // namespace control

#endif  // CONTROL_SCHEDULE_RUNNER_H
