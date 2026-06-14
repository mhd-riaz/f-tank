#ifndef NETWORK_RECONNECT_BACKOFF_H
#define NETWORK_RECONNECT_BACKOFF_H

#include <cstdint>

/**
 * @file ReconnectBackoff.h
 * @brief Pure exponential-backoff calculator for WiFi reconnects (testable).
 */
namespace network {

/// Exponential backoff with a cap: base * 2^attempt, clamped to maxMs.
inline uint32_t backoffDelayMs(uint8_t attempt, uint32_t baseMs, uint32_t maxMs) {
    uint32_t delay = baseMs;
    for (uint8_t i = 0; i < attempt && delay < maxMs; ++i) {
        delay <<= 1;
    }
    return delay > maxMs ? maxMs : delay;
}

}  // namespace network

#endif  // NETWORK_RECONNECT_BACKOFF_H
