#include "channel/ChannelController.h"

#include <Arduino.h>

namespace channel {

ChannelError ChannelController::begin(const ChannelConfig* configs, const bool* initialEnergized,
                                      uint8_t count) {
    if (count > kMaxChannels) {
        return ChannelError::kTooManyChannels;
    }

    count_ = count;
    for (uint8_t i = 0; i < count_; ++i) {
        configs_[i] = configs[i];
        const bool energized = initialEnergized[i];
        energized_[i] = energized;

        const bool level = gpioLevelFor(energized, configs_[i].polarity);
        // Latch the level BEFORE enabling output so the pin comes up correct
        // with no transient toggle (FR-13 anti-flicker).
        digitalWrite(configs_[i].gpio, level ? HIGH : LOW);
        pinMode(configs_[i].gpio, OUTPUT);
        digitalWrite(configs_[i].gpio, level ? HIGH : LOW);
    }
    return ChannelError::kOk;
}

ChannelError ChannelController::applyState(uint8_t index, bool energized) {
    if (index >= count_) {
        return ChannelError::kIndexOutOfRange;
    }
    energized_[index] = energized;
    const bool level = gpioLevelFor(energized, configs_[index].polarity);
    digitalWrite(configs_[index].gpio, level ? HIGH : LOW);
    return ChannelError::kOk;
}

bool ChannelController::isEnergized(uint8_t index) const {
    return index < count_ ? energized_[index] : false;
}

uint8_t ChannelController::cutOverTempTargets() {
    uint8_t cut = 0;
    for (uint8_t i = 0; i < count_; ++i) {
        if (configs_[i].cutOnOverTemp && energized_[i]) {
            applyState(i, false);
            ++cut;
        }
    }
    return cut;
}

}  // namespace channel
