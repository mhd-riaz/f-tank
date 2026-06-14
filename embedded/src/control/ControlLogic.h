#ifndef CONTROL_CONTROL_LOGIC_H
#define CONTROL_CONTROL_LOGIC_H

#include <cstdint>

#include "channel/ChannelController.h"  // for kMaxChannels

/**
 * @file ControlLogic.h
 * @brief Pure, hardware-free decision helpers shared by the control modules.
 *
 * Extracted from ScheduleRunner / SafetyController so the bit-mask, timing, and
 * safety-decision logic is unit-tested natively (the modules themselves touch
 * GPIO and can't run under env:native). All helpers are total functions with
 * explicit bounds so an out-of-range channel index can never shift past the
 * 16-bit mask width (NFR-8, vulnerability-protection).
 */
namespace control {

/// True once `nowMs` is at least `intervalMs` past `lastMs` (stagger gate).
/// Uses unsigned wrap-around subtraction so it is millis()-overflow safe.
inline bool staggerElapsed(uint32_t nowMs, uint32_t lastMs, uint32_t intervalMs) {
    return (nowMs - lastMs) >= intervalMs;
}

/// True if channel `index` is in range to occupy a bit in a 16-bit mask.
inline bool isMaskableIndex(uint8_t index) {
    return index < channel::kMaxChannels;  // kMaxChannels (16) == mask width
}

/// Set or clear bit `index` in `mask`. Out-of-range indices are a no-op (guard).
inline uint16_t setMaskBit(uint16_t mask, uint8_t index, bool on) {
    if (!isMaskableIndex(index)) {
        return mask;
    }
    const uint16_t bit = static_cast<uint16_t>(1u << index);
    return on ? static_cast<uint16_t>(mask | bit) : static_cast<uint16_t>(mask & ~bit);
}

/// True if bit `index` is set in `mask`. Out-of-range indices read as false.
inline bool maskBit(uint16_t mask, uint8_t index) {
    if (!isMaskableIndex(index)) {
        return false;
    }
    return (mask & static_cast<uint16_t>(1u << index)) != 0;
}

/// Resolve a channel's commanded output: a force-off override beats the schedule.
inline bool resolveChannelOutput(bool scheduled, bool forcedOff) {
    return forcedOff ? false : scheduled;
}

/// Heater-cut decision: cut on a sensor fault (can't verify temp -> fail safe)
/// or while the high-temperature band is active (FR-23/24). Low temp never cuts.
inline bool shouldCutHeater(bool sensorFault, bool highActive) {
    return sensorFault || highActive;
}

}  // namespace control

#endif  // CONTROL_CONTROL_LOGIC_H
