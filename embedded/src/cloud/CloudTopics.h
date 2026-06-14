#ifndef CLOUD_CLOUD_TOPICS_H
#define CLOUD_CLOUD_TOPICS_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

/**
 * @file CloudTopics.h
 * @brief Pure, injection-safe MQTT topic construction (hardware-free, testable).
 *
 * Topics are scoped by device ID: `ftank/<deviceId>/<suffix>`. The device ID is
 * strictly validated (alphanumeric, '-' and '_' only) BEFORE being placed into
 * a topic so a malicious/garbled ID can never inject wildcards ('+', '#'),
 * separators, or overflow the buffer (NFR-8, vulnerability-protection).
 */
namespace cloud {

constexpr char kTopicRoot[] = "ftank";
constexpr uint8_t kDeviceIdMaxLen = 32;  ///< excluding null terminator
constexpr size_t kTopicBufSize = 96;     ///< fits root + id + longest suffix

/// True if `id` is non-empty, within length, and only [A-Za-z0-9_-].
inline bool isValidDeviceId(const char* id) {
    if (id == nullptr) {
        return false;
    }
    const size_t len = strlen(id);
    if (len == 0 || len > kDeviceIdMaxLen) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        const char c = id[i];
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) {
            return false;  // rejects '/', '+', '#', spaces, control chars
        }
    }
    return true;
}

/// Build `ftank/<deviceId>/<suffix>` into `out`. Returns false on bad id/overflow.
inline bool buildTopic(const char* deviceId, const char* suffix, char* out, size_t outSize) {
    if (!isValidDeviceId(deviceId) || suffix == nullptr || out == nullptr) {
        return false;
    }
    const int n = snprintf(out, outSize, "%s/%s/%s", kTopicRoot, deviceId, suffix);
    return n > 0 && static_cast<size_t>(n) < outSize;
}

// Canonical topic suffixes.
constexpr char kSuffixTelemetry[] = "telemetry";       ///< device -> cloud
constexpr char kSuffixStatus[] = "status";             ///< device -> cloud (retained)
constexpr char kSuffixConfigState[] = "config/state";  ///< device -> cloud (retained)
constexpr char kSuffixConfigSet[] = "config/set";      ///< cloud -> device
constexpr char kSuffixCommand[] = "cmd";               ///< cloud -> device

}  // namespace cloud

#endif  // CLOUD_CLOUD_TOPICS_H
