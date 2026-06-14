#ifndef OTA_FIRMWARE_VERSION_H
#define OTA_FIRMWARE_VERSION_H

#include <cstdint>
#include <cstdio>
#include <cstring>

/**
 * @file FirmwareVersion.h
 * @brief Pure semantic-version parse + compare (hardware-free, testable).
 *
 * Used to reject OTA images that are not strictly newer (anti-rollback policy
 * at the application layer, complementing bootloader rollback).
 */
namespace ota {

struct FirmwareVersion {
    uint16_t major = 0;
    uint16_t minor = 0;
    uint16_t patch = 0;
};

/// Parse "MAJOR.MINOR.PATCH" (ignores any "-suffix"). Returns false on garbage.
inline bool parseVersion(const char* str, FirmwareVersion& out) {
    if (str == nullptr) {
        return false;
    }
    unsigned a = 0, b = 0, c = 0;
    // Bounded, numeric-only parse; %u stops at the first non-digit.
    if (sscanf(str, "%u.%u.%u", &a, &b, &c) != 3) {
        return false;
    }
    if (a > 0xFFFF || b > 0xFFFF || c > 0xFFFF) {
        return false;
    }
    out.major = static_cast<uint16_t>(a);
    out.minor = static_cast<uint16_t>(b);
    out.patch = static_cast<uint16_t>(c);
    return true;
}

/// Returns <0 if a<b, 0 if equal, >0 if a>b.
inline int compareVersion(const FirmwareVersion& a, const FirmwareVersion& b) {
    if (a.major != b.major) {
        return a.major < b.major ? -1 : 1;
    }
    if (a.minor != b.minor) {
        return a.minor < b.minor ? -1 : 1;
    }
    if (a.patch != b.patch) {
        return a.patch < b.patch ? -1 : 1;
    }
    return 0;
}

/// True if `candidate` is strictly newer than `current` (OTA acceptance rule).
inline bool isNewer(const FirmwareVersion& candidate, const FirmwareVersion& current) {
    return compareVersion(candidate, current) > 0;
}

}  // namespace ota

#endif  // OTA_FIRMWARE_VERSION_H
