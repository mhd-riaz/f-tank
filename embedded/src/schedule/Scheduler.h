#ifndef SCHEDULE_SCHEDULER_H
#define SCHEDULE_SCHEDULER_H

#include "channel/ChannelController.h"  // for kMaxChannels
#include "schedule/Schedule.h"

/**
 * @file Scheduler.h
 * @brief Holds per-channel schedules and computes desired channel states.
 *
 * Pure logic (no hardware) so it runs under the native test environment. The
 * orchestrator applies the computed states to hardware with load staggering.
 */
namespace schedule {

class Scheduler {
  public:
    /// Set the active channel count (clamped to kMaxChannels).
    void setChannelCount(uint8_t count) {
        channelCount_ = count <= channel::kMaxChannels ? count : channel::kMaxChannels;
    }

    uint8_t channelCount() const {
        return channelCount_;
    }

    /// Mutable access to a channel's schedule for configuration/CRUD.
    ChannelSchedule& schedule(uint8_t index) {
        return schedules_[index];
    }
    const ChannelSchedule& schedule(uint8_t index) const {
        return schedules_[index];
    }

    /// Desired energized state for one channel at the given minute-of-day.
    bool desiredState(uint8_t index, uint16_t minuteOfDay) const {
        if (index >= channelCount_) {
            return false;
        }
        return isEnergizedAt(schedules_[index], minuteOfDay);
    }

    /// Compute desired states for all channels into `out` (size >= channelCount).
    void computeAll(uint16_t minuteOfDay, bool* out) const {
        for (uint8_t i = 0; i < channelCount_; ++i) {
            out[i] = isEnergizedAt(schedules_[i], minuteOfDay);
        }
    }

  private:
    ChannelSchedule schedules_[channel::kMaxChannels];
    uint8_t channelCount_ = 0;
};

}  // namespace schedule

#endif  // SCHEDULE_SCHEDULER_H
