#ifndef NETWORK_NETWORK_STORE_H
#define NETWORK_NETWORK_STORE_H

#include <cstdint>

/**
 * @file NetworkStore.h
 * @brief Secure NVS store for WiFi credentials + device token (NFR-5).
 *
 * Kept in a SEPARATE NVS namespace from the device config so secrets are never
 * serialized into the API-readable config blob. Populated by BLE provisioning
 * (M2) and read by WiFiManager / the local API auth layer.
 */
namespace network {

constexpr uint8_t kSsidSize = 33;      ///< 32 chars + null (IEEE 802.11 max)
constexpr uint8_t kPasswordSize = 64;  ///< 63 chars + null (WPA2 PSK max)
constexpr uint8_t kTokenSize = 49;     ///< 48-char bearer token + null

class NetworkStore {
  public:
    /// Load credentials/token from NVS. Safe to call once at boot.
    void begin();

    bool hasWifiCredentials() const {
        return ssid_[0] != '\0';
    }
    const char* ssid() const {
        return ssid_;
    }
    const char* password() const {
        return password_;
    }

    /// Persist WiFi credentials (validated/bounded by caller). Returns success.
    bool setWifiCredentials(const char* ssid, const char* password);

    bool hasToken() const {
        return token_[0] != '\0';
    }
    const char* token() const {
        return token_;
    }

    /// Persist the device bearer token (issued during provisioning).
    bool setToken(const char* token);

    /// Erase all stored network secrets (factory reset / forget WiFi, FR-10).
    void clear();

  private:
    char ssid_[kSsidSize] = {0};
    char password_[kPasswordSize] = {0};
    char token_[kTokenSize] = {0};
};

}  // namespace network

#endif  // NETWORK_NETWORK_STORE_H
