#ifndef STORAGE_PERSISTENT_CONFIG_H
#define STORAGE_PERSISTENT_CONFIG_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "alert/AlertTypes.h"
#include "channel/ChannelConfig.h"
#include "channel/ChannelController.h"
#include "schedule/Schedule.h"

/**
 * @file PersistentConfig.h
 * @brief Versioned, fixed-size device config + pure (de)serialization.
 *
 * Hardware-free so pack/unpack/validation are unit-tested natively. The blob is
 * fixed-size (simple, robust, ~constant NVS footprint) and CRC-protected. On a
 * version mismatch or CRC failure the caller re-seeds factory defaults
 * (discard-and-reseed; migrations added later if the schema changes).
 */
namespace storage {

/// Bump when the on-flash layout changes (triggers discard + re-seed).
constexpr uint16_t kConfigVersion = 2;

/// Max POSIX TZ string length incl. null (e.g. "AEST-10AEDT,M10.1.0,M4.1.0/3").
constexpr uint8_t kTzStringSize = 48;

/// Per-channel persisted settings (config + schedule together).
struct PersistentChannel {
    channel::ChannelConfig config;
    schedule::ChannelSchedule schedule;
};

/// Whole-device persisted configuration.
struct PersistentConfig {
    uint16_t version = kConfigVersion;
    uint8_t channelCount = 0;
    char tz[kTzStringSize] = "UTC0";  ///< POSIX TZ string, DST-aware (FR-7a)
    alert::TempThresholds thresholds;
    PersistentChannel channels[channel::kMaxChannels];
};

/// CRC-32 (IEEE 802.3, reflected) over a byte span. 0 seed.
inline uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            const uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

/// Serialized blob = [PersistentConfig][crc32]. Fixed size.
constexpr size_t kBlobSize = sizeof(PersistentConfig) + sizeof(uint32_t);

/// Pack config + trailing CRC into out (>= kBlobSize). Returns bytes written.
inline size_t pack(const PersistentConfig& cfg, uint8_t* out, size_t outSize) {
    if (outSize < kBlobSize) {
        return 0;
    }
    memcpy(out, &cfg, sizeof(cfg));
    const uint32_t crc = crc32(out, sizeof(cfg));
    memcpy(out + sizeof(cfg), &crc, sizeof(crc));
    return kBlobSize;
}

/// Validate + unpack a blob. Returns true only if size, version, and CRC pass.
inline bool unpack(const uint8_t* in, size_t inSize, PersistentConfig& outCfg) {
    if (inSize != kBlobSize) {
        return false;
    }
    uint32_t storedCrc = 0;
    memcpy(&storedCrc, in + sizeof(PersistentConfig), sizeof(storedCrc));
    if (crc32(in, sizeof(PersistentConfig)) != storedCrc) {
        return false;  // corrupt
    }
    PersistentConfig tmp;
    memcpy(&tmp, in, sizeof(tmp));
    if (tmp.version != kConfigVersion) {
        return false;  // discard + re-seed on version mismatch
    }
    if (tmp.channelCount > channel::kMaxChannels) {
        return false;  // bounds guard
    }
    outCfg = tmp;
    return true;
}

}  // namespace storage

#endif  // STORAGE_PERSISTENT_CONFIG_H
