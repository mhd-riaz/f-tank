#include "network/WiFiManager.h"

#include <WiFi.h>

#include "network/ReconnectBackoff.h"

namespace network {
namespace {

constexpr uint32_t kConnectTimeoutMs = 15000;  // give up an attempt after 15 s
constexpr uint32_t kBackoffBaseMs = 1000;      // first retry after ~1 s
constexpr uint32_t kBackoffMaxMs = 60000;      // cap retries at 60 s
constexpr uint8_t kMaxBackoffAttempt = 6;      // attempt counter ceiling

}  // namespace

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // we drive reconnect ourselves (deterministic)
    if (store_.hasWifiCredentials()) {
        startAttempt(millis());
    } else {
        state_ = WifiState::kUnprovisioned;
    }
}

void WiFiManager::reconfigure() {
    WiFi.disconnect(/*wifioff=*/false, /*eraseap=*/true);
    attempt_ = 0;
    if (store_.hasWifiCredentials()) {
        startAttempt(millis());
    } else {
        state_ = WifiState::kUnprovisioned;
    }
}

void WiFiManager::startAttempt(uint32_t nowMs) {
    WiFi.begin(store_.ssid(), store_.password());
    state_ = WifiState::kConnecting;
    phaseStartMs_ = nowMs;
}

void WiFiManager::update(uint32_t nowMs) {
    switch (state_) {
        case WifiState::kUnprovisioned:
            if (store_.hasWifiCredentials()) {
                attempt_ = 0;
                startAttempt(nowMs);
            }
            break;

        case WifiState::kConnecting:
            if (WiFi.status() == WL_CONNECTED) {
                state_ = WifiState::kConnected;
                attempt_ = 0;
                break;
            }
            if (nowMs - phaseStartMs_ >= kConnectTimeoutMs) {
                WiFi.disconnect();
                if (attempt_ < kMaxBackoffAttempt) {
                    ++attempt_;
                }
                backoffMs_ = backoffDelayMs(attempt_, kBackoffBaseMs, kBackoffMaxMs);
                phaseStartMs_ = nowMs;
                state_ = WifiState::kBackoff;
            }
            break;

        case WifiState::kConnected:
            if (WiFi.status() != WL_CONNECTED) {
                attempt_ = 0;  // immediate first reconnect attempt on a drop
                startAttempt(nowMs);
            }
            break;

        case WifiState::kBackoff:
            if (nowMs - phaseStartMs_ >= backoffMs_) {
                startAttempt(nowMs);
            }
            break;
    }
}

}  // namespace network
