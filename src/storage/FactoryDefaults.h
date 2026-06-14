#ifndef STORAGE_FACTORY_DEFAULTS_H
#define STORAGE_FACTORY_DEFAULTS_H

#include <cstdio>

#include "config/pins.h"
#include "storage/PersistentConfig.h"

/**
 * @file FactoryDefaults.h
 * @brief Seeds a PersistentConfig with the legacy intentional defaults.
 *
 * Hardware-free (uses only the pin map constants) so it is native-testable and
 * shared between first-boot seeding and version-mismatch re-seeding.
 */
namespace storage {

inline void seedFactoryDefaults(PersistentConfig& cfg) {
    cfg = PersistentConfig{};  // reset to zeroed defaults
    cfg.version = kConfigVersion;

    const uint8_t count = pins::kRelayChannelCount;
    cfg.channelCount = count;
    cfg.thresholds = alert::TempThresholds{};  // low 20, high 31, hyst 0.5

    static const char* const kNames[] = {"Light", "CO2", "Heater", "Pump"};

    for (uint8_t i = 0; i < count; ++i) {
        PersistentChannel& ch = cfg.channels[i];
        ch.config.gpio = pins::kRelayChannels[i];
        ch.config.polarity = channel::RelayPolarity::kActiveLow;  // provisioned per device
        ch.config.enabled = true;
        ch.config.cutOnOverTemp = (i == 2);  // Heater = default cut target (FR-24)
        snprintf(ch.config.name, sizeof(ch.config.name), "%s", i < 4 ? kNames[i] : "Channel");
        ch.schedule.windowCount = 0;
        ch.schedule.inverted = false;
    }

    if (count > 0) {  // Light: 10:30-14:30, 17:00-20:00
        auto& s = cfg.channels[0].schedule;
        s.windows[0] = schedule::TimeWindow(630, 870);
        s.windows[1] = schedule::TimeWindow(1020, 1200);
        s.windowCount = 2;
    }
    if (count > 1) {  // CO2: 7:30-13:30, 14:30-20:30
        auto& s = cfg.channels[1].schedule;
        s.windows[0] = schedule::TimeWindow(450, 810);
        s.windows[1] = schedule::TimeWindow(870, 1230);
        s.windowCount = 2;
    }
    if (count > 2) {  // Heater: 0:00-4:00, 14:31-16:31
        auto& s = cfg.channels[2].schedule;
        s.windows[0] = schedule::TimeWindow(0, 240);
        s.windows[1] = schedule::TimeWindow(871, 991);
        s.windowCount = 2;
    }
    if (count > 3) {  // Pump: ON 24h except 9:30-10:30, 14:00-15:00
        auto& s = cfg.channels[3].schedule;
        s.windows[0] = schedule::TimeWindow(570, 630);
        s.windows[1] = schedule::TimeWindow(840, 900);
        s.windowCount = 2;
        s.inverted = true;
    }
}

}  // namespace storage

#endif  // STORAGE_FACTORY_DEFAULTS_H
