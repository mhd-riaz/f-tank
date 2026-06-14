#ifndef CLOUD_CLOUD_STORE_H
#define CLOUD_CLOUD_STORE_H

#include <cstdint>

/**
 * @file CloudStore.h
 * @brief Secure NVS store for cloud connection config + X.509 material (NFR-5).
 *
 * Separate NVS namespace from device config and WiFi creds. Holds the broker
 * endpoint, device ID, subscription flag, and the mutual-TLS credentials
 * (CA cert, client cert, client private key) provisioned at manufacture /
 * onboarding. Secrets are never exposed via the local API.
 */
namespace cloud {

constexpr uint16_t kEndpointSize = 128;  ///< broker host[:path], + null
constexpr uint8_t kDeviceIdSize = 33;    ///< 32 chars + null (matches topics)
constexpr uint16_t kCertSize = 2048;     ///< PEM cert/key max, + null

class CloudStore {
  public:
    /// Load cloud config from NVS. Safe to call once at boot.
    void begin();

    bool isProvisioned() const;

    const char* endpoint() const {
        return endpoint_;
    }
    uint16_t port() const {
        return port_;
    }
    const char* deviceId() const {
        return deviceId_;
    }

    /// Subscription gate: cloud link only runs when true (Subscription tier).
    bool subscriptionActive() const {
        return subscriptionActive_;
    }
    void setSubscriptionActive(bool active);

    const char* caCert() const {
        return caCert_;
    }
    const char* clientCert() const {
        return clientCert_;
    }
    const char* clientKey() const {
        return clientKey_;
    }
    bool hasTlsMaterial() const {
        return caCert_[0] != '\0' && clientCert_[0] != '\0' && clientKey_[0] != '\0';
    }

    /// Provision endpoint + identity (validated/bounded by caller). Returns ok.
    bool setEndpoint(const char* host, uint16_t port, const char* deviceId);
    bool setTlsMaterial(const char* caCert, const char* clientCert, const char* clientKey);

    /// Erase all cloud secrets (factory reset).
    void clear();

  private:
    char endpoint_[kEndpointSize] = {0};
    uint16_t port_ = 8883;
    char deviceId_[kDeviceIdSize] = {0};
    bool subscriptionActive_ = false;

    char caCert_[kCertSize] = {0};
    char clientCert_[kCertSize] = {0};
    char clientKey_[kCertSize] = {0};
};

}  // namespace cloud

#endif  // CLOUD_CLOUD_STORE_H
