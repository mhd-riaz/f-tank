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
#include "api/ConfigBroker.h"
#include "api/LocalApiServer.h"
#include "api/StatusBroker.h"
#include "api/TokenGenerator.h"
#include "channel/ChannelController.h"
#include "cloud/CloudClient.h"
#include "cloud/CloudStore.h"
#include "config/features.h"
#include "config/pins.h"
#include "control/SafetyController.h"
#include "control/ScheduleRunner.h"
#include "display/DisplayManager.h"
#include "network/NetworkStore.h"
#include "network/NtpClient.h"
#include "network/WiFiManager.h"
#include "ota/OtaUpdater.h"
#include "provisioning/BleProvisioner.h"
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
api::ConfigBroker configBroker;
api::StatusBroker statusBroker;
ota::OtaUpdater otaUpdater;
api::LocalApiServer apiServer(networkStore, configBroker, statusBroker, otaUpdater);
provisioning::BleProvisioner bleProvisioner(networkStore, configStore);
#if FT_FEATURE_CLOUD
cloud::CloudStore cloudStore;
cloud::CloudClient cloudClient(cloudStore, configBroker, statusBroker, configStore);
bool cloudStarted = false;
#endif

bool wifiWasConnected = false;
bool ntpConfigured = false;
bool apiStarted = false;
bool otaBootConfirmed = false;

// Uptime after which a freshly-OTA'd image is considered healthy and confirmed
// (cancels bootloader rollback). Long enough to clear init + first network use.
constexpr uint32_t kOtaHealthyConfirmMs = 30000;

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

// Build and publish a status snapshot for the API/WebSocket (AD-1).
void publishStatus(uint32_t nowMs) {
    (void)nowMs;
    api::StatusSnapshot snap;
    snap.temperatureC = temperature.celsius();
    snap.temperatureValid = temperature.hasValidReading();
    snap.activeAlertMask = alerts.activeMask();
    snap.timeSource = static_cast<uint8_t>(timeService.source());
    snap.wifiState = static_cast<uint8_t>(wifiManager.state());
    snap.minuteOfDay = timeService.minutesSinceMidnight();
    snap.channelCount = channels.count();
    for (uint8_t i = 0; i < channels.count(); ++i) {
        snap.channelEnergized[i] = channels.isEnergized(i);
    }
    statusBroker.publish(snap);
}

// Apply a config staged by the API: persist, re-apply to hardware, republish.
void applyStagedConfig() {
    storage::PersistentConfig incoming;
    if (!configBroker.consume(incoming)) {
        return;
    }
    configStore.config() = incoming;
    configStore.save();  // write-on-change; persists user edits (FR-34)
    applyConfig(timeService.minutesSinceMidnight());
    apiServer.publishConfig(configStore.config());
}

// Issue a device bearer token on first use if none exists yet. BLE provisioning
// will own this in production; this keeps the API usable for development.
void ensureDeviceToken() {
    if (networkStore.hasToken()) {
        return;
    }
    char token[network::kTokenSize] = {0};
    if (api::generateToken(token, sizeof(token)) && networkStore.setToken(token)) {
        Serial.printf("Device token (dev): %s\n", token);
    }
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
    ensureDeviceToken();
    maybeSeedDevWifi();
    configBroker.begin();
    statusBroker.begin();
    apiServer.publishConfig(configStore.config());
#if FT_FEATURE_CLOUD
    cloudStore.begin();
#endif
    // BLE provisioning advertises only when no WiFi creds exist yet (AD-6).
    bleProvisioner.beginIfUnprovisioned();
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

    // BLE provisioning: when the app finishes, switch WiFi to the new creds.
    bleProvisioner.update(nowMs);
    if (bleProvisioner.justProvisioned()) {
        bleProvisioner.clearProvisionedFlag();
        ntpConfigured = false;  // re-arm NTP for the new connection
        wifiManager.reconfigure();
    }

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

    // Start the local API once WiFi is up; then service it (AD-3/AD-5).
    if (wifiConnected && !apiStarted) {
        apiServer.begin();
        apiStarted = true;
    }
    if (apiStarted) {
        apiServer.loop(nowMs);
    }

#if FT_FEATURE_CLOUD
    // Cloud link (Subscription tier, 8 MB SKU). Additive; never blocks control.
    if (wifiConnected) {
        if (!cloudStarted) {
            cloudClient.begin();
            cloudStarted = true;
        }
        cloudClient.update(nowMs);
    }
#endif

    // Confirm a freshly-OTA'd image once it has run healthy for a while; until
    // then the bootloader would roll back on a crash (FR-38/39).
    if (!otaBootConfirmed && nowMs >= kOtaHealthyConfirmMs) {
        ota::OtaUpdater::confirmRunningImageValid();
        otaBootConfirmed = true;
    }
    // A verified OTA image is staged -> reboot into it (flicker-free on boot).
    if (apiServer.otaRebootPending()) {
        Serial.println("OTA verified; rebooting into new image...");
        Serial.flush();
        ESP.restart();
    }

    // Apply any config staged by the API and publish live status (AD-1).
    applyStagedConfig();
    publishStatus(nowMs);

    alerts.update(nowMs);
    displayManager.update(nowMs);
}
