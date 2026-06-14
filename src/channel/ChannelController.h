#ifndef CHANNEL_CHANNEL_CONTROLLER_H
#define CHANNEL_CHANNEL_CONTROLLER_H

#include "channel/ChannelConfig.h"

/**
 * @file ChannelController.h
 * @brief Owns relay channels and drives polarity-correct GPIO levels.
 *
 * Hardware boundary; pure mapping lives in RelayPolarity.h. Keeps logical
 * intent separate from electrical level (NFR-11) and supports glitch-free boot
 * (FR-13): the initial state is latched before outputs are enabled.
 */
namespace channel {

/// Upper bound on channels — sized for the largest hardware variant (Pro, 16).
constexpr uint8_t kMaxChannels = 16;

enum class ChannelError : uint8_t { kOk, kIndexOutOfRange, kTooManyChannels };

class ChannelController {
  public:
    /**
     * @brief Configure outputs and drive the initial state WITHOUT flicker.
     *
     * For each channel the correct GPIO level is latched first, then the pin is
     * switched to OUTPUT — so normally-on loads (e.g. pump) never blip at boot.
     *
     * @param configs          channel configs (copied internally)
     * @param initialEnergized desired on/off per channel for the first drive
     * @param count            number of channels (<= kMaxChannels)
     */
    ChannelError begin(const ChannelConfig* configs, const bool* initialEnergized, uint8_t count);

    /// Drive a single channel to a logical state (polarity-applied). Bounds-checked.
    ChannelError applyState(uint8_t index, bool energized);

    uint8_t count() const {
        return count_;
    }
    bool isEnergized(uint8_t index) const;
    const ChannelConfig& config(uint8_t index) const {
        return configs_[index];
    }

    /// De-energize every channel flagged cutOnOverTemp (safety action, FR-23/24).
    /// Returns the number of channels cut.
    uint8_t cutOverTempTargets();

  private:
    ChannelConfig configs_[kMaxChannels];
    bool energized_[kMaxChannels] = {false};
    uint8_t count_ = 0;
};

}  // namespace channel

#endif  // CHANNEL_CHANNEL_CONTROLLER_H
