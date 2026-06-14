#include "control/ScheduleRunner.h"

#include "control/ControlLogic.h"

namespace control {

void ScheduleRunner::setForceOff(uint8_t index, bool forceOff) {
    forceOffMask_ = setMaskBit(forceOffMask_, index, forceOff);
}

void ScheduleRunner::update(uint16_t minuteOfDay, uint32_t nowMs) {
    if (!staggerElapsed(nowMs, lastStaggerMs_, kStaggerIntervalMs)) {
        return;
    }
    const uint8_t count = channels_.count();
    for (uint8_t i = 0; i < count; ++i) {
        const bool forcedOff = maskBit(forceOffMask_, i);
        const bool desired =
            resolveChannelOutput(scheduler_.desiredState(i, minuteOfDay), forcedOff);
        if (channels_.isEnergized(i) != desired) {
            channels_.applyState(i, desired);
            lastStaggerMs_ = nowMs;  // one transition per interval (FR-16)
            return;
        }
    }
}

}  // namespace control
