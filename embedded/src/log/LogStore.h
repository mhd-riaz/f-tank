#ifndef LOG_LOG_STORE_H
#define LOG_LOG_STORE_H

#include <cstdint>

#include "log/LogTypes.h"

/**
 * @file LogStore.h
 * @brief Persistent, fixed-capacity circular event log on LittleFS (FR-30).
 *
 * Premium / 8 MB SKU only (compiled out when FT_FEATURE_SD_LOG == 0). Backs the
 * reserved `littlefs` data partition with a single pre-allocated ring file so
 * appends are O(1) seek+write with bounded, wear-levelled flash usage
 * (storage-optimization). Each slot is a CRC-protected LogRecord; a small header
 * tracks the write head + count and is itself CRC-checked.
 */
namespace logging {

/// Records retained on flash before the oldest is overwritten.
constexpr uint16_t kFlashCapacity = 1024;

class LogStore {
  public:
    /// Mount LittleFS and load/validate the ring header (format on first use or
    /// version/CRC mismatch). Returns true when the store is usable.
    bool begin();

    /// True once the filesystem is mounted and the ring file is ready.
    bool available() const {
        return ready_;
    }

    /// Append one record, overwriting the oldest when full. Returns true on success.
    bool append(const LogRecord& r);

    /// Records currently retained (<= kFlashCapacity).
    uint16_t count() const {
        return count_;
    }

    uint16_t capacity() const {
        return kFlashCapacity;
    }

    /// Copy up to `max` most-recent records, oldest-first, into out. Returns count.
    uint16_t readRecent(LogRecord* out, uint16_t max) const;

    /// Erase all retained records (reset the ring header).
    bool clear();

  private:
    bool writeHeader();

    bool ready_ = false;
    uint16_t head_ = 0;   ///< next slot index to write
    uint16_t count_ = 0;  ///< retained records
};

}  // namespace logging

#endif  // LOG_LOG_STORE_H
