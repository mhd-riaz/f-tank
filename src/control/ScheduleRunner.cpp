#include "control/ScheduleRunner.h"

namespace control {

void ScheduleRunner::setForceOff(uint8_t index, bool forceOff) {
    const uint16_t bit = static_cast<uint16_t>(1u << index);
    if (forceOff) {
        forceOffMask_ |= bit;
    } else {
        forceOffMask_ &= static_cast<uint16_t>(~bit);
    }
}

void ScheduleRunner::update(uint16_t minuteOfDay, uint32_t nowMs) {
    if (nowMs - lastStaggerMs_ < kStaggerIntervalMs) {
        return;
    }
    const uint8_t count = channels_.count();
    for (uint8_t i = 0; i < count; ++i) {
        const bool forcedOff = (forceOffMask_ & (1u << i)) != 0;
        const bool desired = forcedOff ? false : scheduler_.desiredState(i, minuteOfDay);
        if (channels_.isEnergized(i) != desired) {
            channels_.applyState(i, desired);
            lastStaggerMs_ = nowMs;  // one transition per interval (FR-16)
            return;
        }
    }
}

}  // namespace control
