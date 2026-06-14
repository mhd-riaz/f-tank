#ifndef CHANNEL_RELAY_POLARITY_H
#define CHANNEL_RELAY_POLARITY_H

#include <cstdint>

/**
 * @file RelayPolarity.h
 * @brief Pure mapping from logical intent (energized?) to GPIO drive level.
 *
 * Polarity describes how the fitted relay coil is driven, NOT the contact form.
 * It is provisioned once at manufacture/registration (FR-12) and is the single
 * place that keeps logical state separate from electrical level (NFR-11), so a
 * schedule/relay mismatch can never cause a glitch. Header-only -> unit-tested
 * under the native environment.
 */
namespace channel {

/// How the GPIO must be driven to ENERGIZE (turn on) the relay.
enum class RelayPolarity : uint8_t {
    kActiveHigh,  ///< drive HIGH to energize (e.g. logic-level / NC-style boards)
    kActiveLow,   ///< drive LOW to energize (typical opto-isolated relay boards)
};

/**
 * @brief GPIO level needed to achieve the requested logical state.
 * @param energized true = relay on (load powered), false = relay off.
 * @return true = write HIGH, false = write LOW.
 */
constexpr bool gpioLevelFor(bool energized, RelayPolarity polarity) {
    return polarity == RelayPolarity::kActiveHigh ? energized : !energized;
}

/// Level that DE-energizes a channel — the defined safe state (NFR-3).
constexpr bool deEnergizedLevel(RelayPolarity polarity) {
    return gpioLevelFor(false, polarity);
}

}  // namespace channel

#endif  // CHANNEL_RELAY_POLARITY_H
