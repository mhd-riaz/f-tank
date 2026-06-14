#ifndef OTA_OTA_TYPES_H
#define OTA_OTA_TYPES_H

#include <cstddef>
#include <cstdint>

/**
 * @file OtaTypes.h
 * @brief Pure OTA state, errors, and header helpers (hardware-free, testable).
 */
namespace ota {

/// SHA-256 digest length in bytes.
constexpr size_t kSha256Len = 32;

/// Max ECDSA-P256 DER signature length (typically <= 72 bytes).
constexpr size_t kMaxSignatureLen = 80;

/// Lifecycle of an OTA session.
enum class OtaState : uint8_t {
    kIdle,         ///< no update in progress
    kReceiving,    ///< streaming image to the inactive slot
    kVerifying,    ///< checking SHA-256 + signature
    kReadyReboot,  ///< verified; boot partition set; reboot pending
    kFailed,       ///< aborted (see OtaError)
};

/// Typed OTA failure reasons (never silent).
enum class OtaError : uint8_t {
    kOk,
    kBusy,            ///< an update is already running
    kBadHeader,       ///< missing/garbled version/sha/signature header
    kNotNewer,        ///< image version not strictly newer (anti-rollback)
    kTooLarge,        ///< image exceeds the partition
    kBeginFailed,     ///< could not open the OTA partition
    kWriteFailed,     ///< flash write error
    kSizeMismatch,    ///< received size != declared size
    kHashMismatch,    ///< SHA-256 of image != declared digest
    kBadSignature,    ///< signature failed verification
    kActivateFailed,  ///< setting the boot partition failed
};

/// Decode a lowercase/uppercase hex string into bytes. Returns false on bad
/// length or non-hex characters. `outLen` is the expected byte count.
inline bool hexDecode(const char* hex, uint8_t* out, size_t outLen) {
    if (hex == nullptr || out == nullptr) {
        return false;
    }
    size_t hexLen = 0;
    while (hex[hexLen] != '\0') {
        ++hexLen;
    }
    if (hexLen != outLen * 2) {
        return false;
    }
    auto nibble = [](char c, uint8_t& v) -> bool {
        if (c >= '0' && c <= '9') {
            v = static_cast<uint8_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            v = static_cast<uint8_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            v = static_cast<uint8_t>(c - 'A' + 10);
        } else {
            return false;
        }
        return true;
    };
    for (size_t i = 0; i < outLen; ++i) {
        uint8_t hi = 0, lo = 0;
        if (!nibble(hex[i * 2], hi) || !nibble(hex[i * 2 + 1], lo)) {
            return false;
        }
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

/// Validate the declared image size against the available partition space.
inline bool isImageSizeValid(uint32_t declaredSize, uint32_t partitionSize) {
    return declaredSize > 0 && declaredSize <= partitionSize;
}

}  // namespace ota

#endif  // OTA_OTA_TYPES_H
