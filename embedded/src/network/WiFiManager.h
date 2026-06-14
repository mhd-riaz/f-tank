#ifndef NETWORK_WIFI_MANAGER_H
#define NETWORK_WIFI_MANAGER_H

#include <cstdint>

#include "network/NetworkStore.h"

/**
 * @file WiFiManager.h
 * @brief Non-blocking WiFi connection state machine with backoff reconnect.
 *
 * Connectivity is additive and MUST NOT block scheduling (FR-1/FR-4): all
 * waiting is timed via millis(), never `delay()`. Self-heals after drops with
 * exponential backoff (NFR-4).
 */
namespace network {

enum class WifiState : uint8_t {
    kUnprovisioned,  ///< no stored credentials
    kConnecting,     ///< association in progress
    kConnected,      ///< got an IP
    kBackoff,        ///< waiting before the next attempt
};

class WiFiManager {
  public:
    explicit WiFiManager(NetworkStore& store) : store_(store) {}

    /// Begin: if credentials exist, start the first connection attempt.
    void begin();

    /// Cooperative servicing; non-blocking. Call frequently.
    void update(uint32_t nowMs);

    WifiState state() const {
        return state_;
    }
    bool isConnected() const {
        return state_ == WifiState::kConnected;
    }

    /// Re-read credentials and (re)start connection (after provisioning).
    void reconfigure();

  private:
    void startAttempt(uint32_t nowMs);

    NetworkStore& store_;
    WifiState state_ = WifiState::kUnprovisioned;
    uint8_t attempt_ = 0;
    uint32_t phaseStartMs_ = 0;
    uint32_t backoffMs_ = 0;
};

}  // namespace network

#endif  // NETWORK_WIFI_MANAGER_H
