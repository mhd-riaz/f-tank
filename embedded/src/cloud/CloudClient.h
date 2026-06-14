#ifndef CLOUD_CLOUD_CLIENT_H
#define CLOUD_CLOUD_CLIENT_H

#include "config/features.h"

#if FT_FEATURE_CLOUD

#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "api/ConfigBroker.h"
#include "api/StatusBroker.h"
#include "cloud/CloudStore.h"
#include "storage/ConfigStore.h"

/**
 * @file CloudClient.h
 * @brief Outbound MQTT-over-TLS cloud link (FR-3, AD-cloud). 8 MB SKU only.
 *
 * The device DIALS OUT to the broker (no inbound port). Mutual-TLS with the
 * provisioned X.509 client cert. Publishes telemetry + retained status; on
 * inbound config it validates and stages into the SAME ConfigBroker apply-queue
 * as the local API — one source of truth. Non-blocking; loss of cloud never
 * affects local control or scheduling (FR-1/FR-4). Gated by subscription state.
 */
namespace cloud {

enum class CloudState : uint8_t {
    kDisabled,    ///< no subscription / not provisioned
    kConnecting,  ///< TLS + MQTT connect in progress
    kConnected,   ///< subscribed + publishing
    kBackoff,     ///< waiting before retry
};

class CloudClient {
  public:
    CloudClient(CloudStore& store, api::ConfigBroker& configBroker, api::StatusBroker& statusBroker,
                storage::ConfigStore& configStore)
        : mqtt_(tls_),
          store_(store),
          configBroker_(configBroker),
          statusBroker_(statusBroker),
          configStore_(configStore) {}

    /// Configure TLS material + broker. Call once after WiFi connects.
    void begin();

    /// Cooperative servicing; non-blocking. Call frequently while WiFi is up.
    void update(uint32_t nowMs);

    CloudState state() const {
        return state_;
    }
    bool isConnected() const {
        return state_ == CloudState::kConnected;
    }

  private:
    void startConnect(uint32_t nowMs);
    bool publishTelemetry(uint32_t nowMs);
    void onMessage(const char* topic, const uint8_t* payload, unsigned int len);
    void handleConfigSet(const uint8_t* payload, unsigned int len);

    WiFiClientSecure tls_;
    PubSubClient mqtt_;
    CloudStore& store_;
    api::ConfigBroker& configBroker_;
    api::StatusBroker& statusBroker_;
    storage::ConfigStore& configStore_;

    CloudState state_ = CloudState::kDisabled;
    bool tlsConfigured_ = false;
    uint8_t attempt_ = 0;
    uint32_t phaseStartMs_ = 0;
    uint32_t backoffMs_ = 0;
    uint32_t lastTelemetryMs_ = 0;
};

}  // namespace cloud

#endif  // FT_FEATURE_CLOUD
#endif  // CLOUD_CLOUD_CLIENT_H
