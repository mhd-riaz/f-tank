#ifndef LOG_LOG_TYPES_H
#define LOG_LOG_TYPES_H

#include <cstddef>
#include <cstdint>

/**
 * @file LogTypes.h
 * @brief Compact, fixed-size event-log record + pure (de)serialization + RAM ring.
 *
 * Hardware-free so the wire format and ring-buffer math are unit-tested natively
 * (code-quality: keep pure logic testable). Records use a compact little-endian
 * binary layout (storage-optimization: avoid bloated text on flash) with a
 * trailing CRC-16 so corruption from a partial write is detected on read-back.
 *
 * Logging is *on-demand* (FR-29): records are produced only while recording is
 * enabled by the app; this file holds only the data types, not the capture gate.
 */
namespace logging {

/// Kinds of events captured by the on-demand logger.
enum class LogEventType : uint8_t {
    kNone = 0,
    kBoot = 1,           ///< device started; arg = reset/boot reason
    kChannelOn = 2,      ///< relay channel energized; arg = channel index
    kChannelOff = 3,     ///< relay channel de-energized; arg = channel index
    kTemperature = 4,    ///< periodic temp sample; centiCelsius = temp * 100
    kAlertSet = 5,       ///< alert raised; arg = alert id, detail = active mask
    kAlertClear = 6,     ///< alert cleared; arg = alert id, detail = active mask
    kTimeSource = 7,     ///< clock authority changed; arg = ftime::TimeSource
    kConfigApplied = 8,  ///< a new config was applied + persisted
};

/// One log entry. Fixed POD layout; values not relevant to a type are 0.
struct LogRecord {
    uint32_t epoch = 0;        ///< local-time Unix seconds; 0 if time untrusted
    uint16_t minuteOfDay = 0;  ///< 0-1439, ordering fallback when epoch is 0
    uint8_t type = 0;          ///< LogEventType
    uint8_t arg = 0;           ///< channel index / alert id / time source
    int16_t centiCelsius = 0;  ///< temperature * 100 (kTemperature only)
    uint16_t detail = 0;       ///< alert mask / state bitfield
};

/// Encoded size on the wire: epoch(4) min(2) type(1) arg(1) centi(2) detail(2) crc(2).
constexpr size_t kRecordWireSize = 14;

/// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over a byte span.
inline uint16_t logCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(static_cast<uint16_t>(data[i]) << 8);
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x8000u) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021u);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

inline void putU16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
}

inline void putU32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
}

inline uint16_t getU16(const uint8_t* p) {
    return static_cast<uint16_t>(static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8));
}

inline uint32_t getU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

/// Serialize a record (+ trailing CRC) into out (>= kRecordWireSize). Returns bytes written.
inline size_t encodeRecord(const LogRecord& r, uint8_t* out, size_t outSize) {
    if (outSize < kRecordWireSize) {
        return 0;
    }
    putU32(out + 0, r.epoch);
    putU16(out + 4, r.minuteOfDay);
    out[6] = r.type;
    out[7] = r.arg;
    putU16(out + 8, static_cast<uint16_t>(r.centiCelsius));
    putU16(out + 10, r.detail);
    putU16(out + 12, logCrc16(out, 12));
    return kRecordWireSize;
}

/// Validate + deserialize a record. Returns false on short input or CRC mismatch.
inline bool decodeRecord(const uint8_t* in, size_t inSize, LogRecord& out) {
    if (inSize < kRecordWireSize) {
        return false;
    }
    if (logCrc16(in, 12) != getU16(in + 12)) {
        return false;  // corrupt / partial write
    }
    out.epoch = getU32(in + 0);
    out.minuteOfDay = getU16(in + 4);
    out.type = in[6];
    out.arg = in[7];
    out.centiCelsius = static_cast<int16_t>(getU16(in + 8));
    out.detail = getU16(in + 10);
    return true;
}

/// In-RAM capacity for the on-demand snapshot/stream buffer (both SKUs).
constexpr uint8_t kLogRingCapacity = 64;

/**
 * @brief Fixed-capacity, overwrite-oldest circular buffer of recent records.
 *
 * Bounded RAM (no heap, no fragmentation per memory-optimization) holding the
 * most recent events for on-demand retrieval over the local API. Pure logic so
 * wrap-around and ordering are unit-tested natively.
 */
class LogRingBuffer {
  public:
    void clear() {
        head_ = 0;
        count_ = 0;
    }

    void push(const LogRecord& r) {
        buf_[head_] = r;
        head_ = static_cast<uint8_t>((head_ + 1) % kLogRingCapacity);
        if (count_ < kLogRingCapacity) {
            ++count_;
        }
    }

    uint8_t size() const {
        return count_;
    }

    bool empty() const {
        return count_ == 0;
    }

    /// Access by logical index where 0 == oldest retained record.
    /// Caller must ensure logicalIndex < size().
    const LogRecord& at(uint8_t logicalIndex) const {
        const uint8_t start =
            static_cast<uint8_t>((head_ + kLogRingCapacity - count_) % kLogRingCapacity);
        const uint8_t phys = static_cast<uint8_t>((start + logicalIndex) % kLogRingCapacity);
        return buf_[phys];
    }

  private:
    LogRecord buf_[kLogRingCapacity];
    uint8_t head_ = 0;   ///< next slot to write
    uint8_t count_ = 0;  ///< retained records (<= capacity)
};

}  // namespace logging

#endif  // LOG_LOG_TYPES_H
