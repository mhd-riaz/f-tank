#ifndef CHANNEL_CHANNEL_CONFIG_H
#define CHANNEL_CHANNEL_CONFIG_H

#include <cstdint>

#include "channel/RelayPolarity.h"

/**
 * @file ChannelConfig.h
 * @brief Per-channel configuration (hardware-free, native-testable).
 */
namespace channel {

/// Max channel display-name length incl. null terminator (bounded, no String).
constexpr uint8_t kChannelNameSize = 24;

/// Configuration for a single relay channel.
struct ChannelConfig {
    char name[kChannelNameSize] = {0};                   ///< user label; ASCII, null-terminated
    uint8_t gpio = 0;                                    ///< physical output pin (from pins.h)
    RelayPolarity polarity = RelayPolarity::kActiveLow;  ///< provisioned once (FR-12)
    bool enabled = false;                                ///< channel participates in scheduling
    bool cutOnOverTemp = false;  ///< heater-cut target, provisioned at onboarding (FR-24)
};

}  // namespace channel

#endif  // CHANNEL_CHANNEL_CONFIG_H
