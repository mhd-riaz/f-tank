/**
 * @file main.cpp
 * @brief f-tank firmware entry point — WIRING ONLY (no business logic here).
 *
 * Per the code-quality instruction, setup()/loop() only wire modules together
 * and service them cooperatively (non-blocking). Feature logic lives in modules
 * under src/ and is added milestone by milestone (see docs/REQUIREMENTS.md).
 *
 * Build: PlatformIO env `esp32dev`. Native logic tests: env `native`.
 */
#include <Arduino.h>

#include "alert/AlertManager.h"
#include "channel/ChannelController.h"
#include "config/pins.h"
#include "control/SafetyController.h"
#include "control/ScheduleRunner.h"
#include "display/DisplayManager.h"
#include "network/NetworkStore.h"
#include "network/NtpClient.h"
#include "network/WiFiManager.h"
#include "schedule/Scheduler.h"
#include "sensor/TemperatureSensor.h"
#include "storage/ConfigStore.h"
#include "time/TimeService.h"

#if defined(__has_include)
#if __has_include("config/secrets.h")
#include "config/secrets.h"
#endif
#endif

namespace {

ftime::TimeService timeService;
channel::ChannelController channels;
schedule::Scheduler scheduler;
control::ScheduleRunner scheduleRunner(scheduler, channels);
sensor::TemperatureSensor temperature(pins::kOneWire);
alert::AlertManager alerts(pins::kBuzzer);
control::SafetyController safety(temperature, alerts, channels, scheduleRunner);
display::DisplayManager displayManager(timeService, temperature, channels, alerts);
storage::ConfigStore configStore;
network::NetworkStore networkStore;
network::WiFiManager wifiManager(networkStore);
network::NtpClient ntpClient(timeService);

bool wifiWasConnected = false;
bool ntpConfigured = false;

// Apply persisted config to scheduler + channels and drive outputs to their
// schedule-correct state with zero flicker (FR-13): states are computed BEFORE
// outputs are enabled. Config is the NVS-backed source of truth (FR-34).
void applyConfig(uint16_t minuteOfDay) {
    const storage::PersistentConfig& cfg = configStore.config();
    const uint8_t count = cfg.channelCount;

    scheduler.setChannelCount(count);
    for (uint8_t i = 0; i < count; ++i) {
        scheduler.schedule(i) = cfg.channels[i].schedule;
    }
    safety.setThresholds(cfg.thresholds);

    channel::ChannelConfig configs[channel::kMaxChannels];
    bool initialEnergized[channel::kMaxChannels] = {false};
    scheduler.computeAll(minuteOfDay, initialEnergized);
    for (uint8_t i = 0; i < count; ++i) {
        configs[i] = cfg.channels[i].config;
    }
    channels.begin(configs, initialEnergized, count);
}

// Seed WiFi credentials from a developer secrets header on first boot only,
// when no credentials have been provisioned yet (BLE provisioning replaces this
// in production). No-op unless config/secrets.h defines a non-empty SSID.
void maybeSeedDevWifi() {
#if defined(FT_DEV_WIFI_SSID)
    if (!networkStore.hasWifiCredentials() && sizeof(FT_DEV_WIFI_SSID) > 1) {
        networkStore.setWifiCredentials(FT_DEV_WIFI_SSID, FT_DEV_WIFI_PASSWORD);
    }
#endif
}

}  // namespace

void setup() {
    Serial.begin(115200);
    pinMode(pins::kBuzzer, OUTPUT);
    digitalWrite(pins::kBuzzer, LOW);

    const ftime::TimeError timeErr = timeService.begin();
    if (timeErr != ftime::TimeError::kOk) {
        // Surfaced via the alert manager below; note degraded time source.
        Serial.printf("TimeService degraded: err=%u source=%u\n", static_cast<unsigned>(timeErr),
                      static_cast<unsigned>(timeService.source()));
    }

    const storage::LoadResult loadResult = configStore.begin();
    Serial.printf("ConfigStore: %u\n", static_cast<unsigned>(loadResult));

    applyConfig(timeService.minutesSinceMidnight());
    temperature.begin();
    alerts.begin();
    safety.begin();
    displayManager.begin();
    // RTC/NTP alert wiring: surface a degraded clock to the user.
    alerts.set(alert::AlertId::kRtcFailure, timeErr != ftime::TimeError::kOk);

    // Connectivity (additive; never blocks scheduling, FR-1/FR-4).
    networkStore.begin();
    maybeSeedDevWifi();
    wifiManager.begin();
}

void loop() {
    const uint32_t nowMs = millis();
    timeService.update();
    temperature.update(nowMs);
    safety.update();
    scheduleRunner.update(timeService.minutesSinceMidnight(), nowMs);

    // Service connectivity and react to WiFi up/down edges.
    wifiManager.update(nowMs);
    const bool wifiConnected = wifiManager.isConnected();
    if (wifiConnected && !wifiWasConnected) {
        if (!ntpConfigured) {
            ntpClient.begin(configStore.config().tz);
            ntpConfigured = true;
        } else {
            ntpClient.onWifiConnected(nowMs);
        }
    }
    wifiWasConnected = wifiConnected;
    if (ntpConfigured && wifiConnected) {
        ntpClient.update(nowMs);
    }
    alerts.set(alert::AlertId::kNtpFailure, ntpClient.inFailure());

    alerts.update(nowMs);
    displayManager.update(nowMs);
}
