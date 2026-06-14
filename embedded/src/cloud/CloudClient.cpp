#include "cloud/CloudClient.h"

#if FT_FEATURE_CLOUD

#include <ArduinoJson.h>

#include "api/ApiValidation.h"
#include "cloud/CloudTopics.h"
#include "network/ReconnectBackoff.h"

namespace cloud {
namespace {

constexpr uint32_t kConnectTimeoutMs = 10000;
constexpr uint32_t kBackoffBaseMs = 2000;
constexpr uint32_t kBackoffMaxMs = 120000;
constexpr uint8_t kMaxBackoffAttempt = 6;
constexpr uint32_t kTelemetryIntervalMs = 15000;
constexpr uint16_t kMqttBufferSize = 2048;  // fits config payloads

CloudClient* g_self = nullptr;

}  // namespace

void CloudClient::begin() {
    if (!store_.isProvisioned()) {
        state_ = CloudState::kDisabled;
        return;
    }
    g_self = this;
    tls_.setCACert(store_.caCert());
    tls_.setCertificate(store_.clientCert());
    tls_.setPrivateKey(store_.clientKey());
    tlsConfigured_ = true;

    mqtt_.setBufferSize(kMqttBufferSize);
    mqtt_.setServer(store_.endpoint(), store_.port());
    mqtt_.setCallback([](char* topic, uint8_t* payload, unsigned int len) {
        if (g_self != nullptr) {
            g_self->onMessage(topic, payload, len);
        }
    });
    state_ = CloudState::kBackoff;  // first update() triggers a connect attempt
    phaseStartMs_ = 0;
    backoffMs_ = 0;
}

void CloudClient::startConnect(uint32_t nowMs) {
    // Retained last-will marks the device offline if the link drops.
    char statusTopic[kTopicBufSize];
    if (!buildTopic(store_.deviceId(), kSuffixStatus, statusTopic, sizeof(statusTopic))) {
        state_ = CloudState::kDisabled;
        return;
    }

    state_ = CloudState::kConnecting;
    phaseStartMs_ = nowMs;

    const bool ok = mqtt_.connect(store_.deviceId(), nullptr, nullptr, statusTopic, /*qos=*/1,
                                  /*retain=*/true, "{\"online\":false}", /*cleanSession=*/true);
    if (!ok) {
        if (attempt_ < kMaxBackoffAttempt) {
            ++attempt_;
        }
        backoffMs_ = network::backoffDelayMs(attempt_, kBackoffBaseMs, kBackoffMaxMs);
        phaseStartMs_ = nowMs;
        state_ = CloudState::kBackoff;
        return;
    }

    // Connected: announce online (retained) and subscribe to inbound topics.
    mqtt_.publish(statusTopic, "{\"online\":true}", /*retained=*/true);

    char configSet[kTopicBufSize];
    char cmd[kTopicBufSize];
    if (buildTopic(store_.deviceId(), kSuffixConfigSet, configSet, sizeof(configSet))) {
        mqtt_.subscribe(configSet, /*qos=*/1);
    }
    if (buildTopic(store_.deviceId(), kSuffixCommand, cmd, sizeof(cmd))) {
        mqtt_.subscribe(cmd, /*qos=*/1);
    }
    attempt_ = 0;
    state_ = CloudState::kConnected;
    lastTelemetryMs_ = nowMs;
}

bool CloudClient::publishTelemetry(uint32_t nowMs) {
    api::StatusSnapshot snap;
    if (!statusBroker_.read(snap)) {
        return false;
    }
    JsonDocument doc;
    doc["tempC"] = snap.temperatureValid ? snap.temperatureC : (float)-127.0;
    doc["tempValid"] = snap.temperatureValid;
    doc["alerts"] = snap.activeAlertMask;
    doc["wifi"] = snap.wifiState;
    doc["minuteOfDay"] = snap.minuteOfDay;
    JsonArray ch = doc["channels"].to<JsonArray>();
    for (uint8_t i = 0; i < snap.channelCount; ++i) {
        ch.add(snap.channelEnergized[i]);
    }

    char topic[kTopicBufSize];
    if (!buildTopic(store_.deviceId(), kSuffixTelemetry, topic, sizeof(topic))) {
        return false;
    }
    char buf[512];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        return false;
    }
    (void)nowMs;
    return mqtt_.publish(topic, reinterpret_cast<const uint8_t*>(buf), n, /*retained=*/false);
}

void CloudClient::onMessage(const char* topic, const uint8_t* payload, unsigned int len) {
    char configSet[kTopicBufSize];
    if (buildTopic(store_.deviceId(), kSuffixConfigSet, configSet, sizeof(configSet)) &&
        strcmp(topic, configSet) == 0) {
        handleConfigSet(payload, len);
    }
    // Commands (kSuffixCommand) handled here in a later iteration.
}

void CloudClient::handleConfigSet(const uint8_t* payload, unsigned int len) {
    JsonDocument doc;
    if (deserializeJson(doc, payload, len) != DeserializationError::Ok) {
        return;  // ignore malformed cloud payloads (NFR-8)
    }

    // Overlay user-editable fields onto the current applied config, validate,
    // and stage into the SAME apply-queue as the local API (single source).
    storage::PersistentConfig next = configStore_.config();

    if (doc["tz"].is<const char*>()) {
        const char* tz = doc["tz"];
        if (!api::isValidTz(tz)) {
            return;
        }
        snprintf(next.tz, sizeof(next.tz), "%s", tz);
    }
    if (doc["thresholds"].is<JsonObject>()) {
        JsonObject th = doc["thresholds"];
        alert::TempThresholds t = next.thresholds;
        if (th["lowC"].is<float>())
            t.lowC = th["lowC"];
        if (th["highC"].is<float>())
            t.highC = th["highC"];
        if (th["hysteresisC"].is<float>())
            t.hysteresisC = th["hysteresisC"];
        if (!api::isValidThresholds(t)) {
            return;
        }
        next.thresholds = t;
    }
    if (doc["channels"].is<JsonArray>()) {
        JsonArray chans = doc["channels"];
        if (chans.size() != next.channelCount) {
            return;
        }
        uint8_t idx = 0;
        for (JsonObject c : chans) {
            storage::PersistentChannel& pc = next.channels[idx];
            if (c["name"].is<const char*>()) {
                const char* name = c["name"];
                if (!api::isValidChannelName(name)) {
                    return;
                }
                snprintf(pc.config.name, sizeof(pc.config.name), "%s", name);
            }
            if (c["enabled"].is<bool>()) {
                pc.config.enabled = c["enabled"];
            }
            if (c["schedule"].is<JsonObject>()) {
                JsonObject sch = c["schedule"];
                schedule::ChannelSchedule ns;
                ns.inverted = sch["inverted"].is<bool>() ? (bool)sch["inverted"] : false;
                ns.windowCount = 0;
                if (sch["windows"].is<JsonArray>()) {
                    JsonArray wins = sch["windows"];
                    if (wins.size() > schedule::kMaxWindowsPerChannel) {
                        return;
                    }
                    for (JsonObject win : wins) {
                        ns.windows[ns.windowCount].startMin =
                            static_cast<uint16_t>(win["start"] | 0);
                        ns.windows[ns.windowCount].endMin = static_cast<uint16_t>(win["end"] | 0);
                        ns.windowCount++;
                    }
                }
                if (!api::isValidSchedule(ns)) {
                    return;
                }
                pc.schedule = ns;
            }
            ++idx;
        }
    }

    configBroker_.stage(next);  // control loop applies + persists (AD-1)
}

void CloudClient::update(uint32_t nowMs) {
    if (!tlsConfigured_ || !store_.isProvisioned()) {
        state_ = CloudState::kDisabled;
        return;
    }
    // Subscription gate: drop the link if the subscription lapsed.
    if (!store_.subscriptionActive()) {
        if (mqtt_.connected()) {
            mqtt_.disconnect();
        }
        state_ = CloudState::kDisabled;
        return;
    }

    switch (state_) {
        case CloudState::kDisabled:
            state_ = CloudState::kBackoff;  // re-arm now that subscription is active
            phaseStartMs_ = nowMs;
            backoffMs_ = 0;
            break;

        case CloudState::kBackoff:
            if (nowMs - phaseStartMs_ >= backoffMs_) {
                startConnect(nowMs);
            }
            break;

        case CloudState::kConnecting:
            // startConnect() resolves synchronously; guard against a stuck state.
            if (nowMs - phaseStartMs_ >= kConnectTimeoutMs) {
                state_ = CloudState::kBackoff;
                phaseStartMs_ = nowMs;
            }
            break;

        case CloudState::kConnected:
            if (!mqtt_.connected()) {
                state_ = CloudState::kBackoff;
                phaseStartMs_ = nowMs;
                backoffMs_ = kBackoffBaseMs;
                break;
            }
            mqtt_.loop();  // non-blocking pump
            if (nowMs - lastTelemetryMs_ >= kTelemetryIntervalMs) {
                lastTelemetryMs_ = nowMs;
                publishTelemetry(nowMs);
            }
            break;
    }
}

}  // namespace cloud

#endif  // FT_FEATURE_CLOUD
