#include "log/LogStore.h"

#include "config/features.h"

#if FT_FEATURE_SD_LOG

#include <FS.h>
#include <LittleFS.h>

namespace logging {
namespace {

constexpr char kLogPath[] = "/ftlog.bin";
constexpr char kPartitionLabel[] = "littlefs";  // matches partitions_8mb.csv
constexpr uint32_t kMagic = 0x46544C47u;        // "FTLG"
constexpr uint16_t kStoreVersion = 1;

// Header layout: magic(4) version(2) capacity(2) head(2) count(2) crc16(2).
constexpr uint32_t kHeaderSize = 14;
constexpr uint32_t kFileSize =
    kHeaderSize + static_cast<uint32_t>(kFlashCapacity) * kRecordWireSize;

uint32_t slotOffset(uint16_t slot) {
    return kHeaderSize + static_cast<uint32_t>(slot) * kRecordWireSize;
}

}  // namespace

bool LogStore::begin() {
    if (ready_) {
        return true;
    }
    if (!LittleFS.begin(/*formatOnFail=*/true, "/littlefs", 10, kPartitionLabel)) {
        return false;
    }

    // Try to load an existing, valid ring header.
    if (LittleFS.exists(kLogPath)) {
        File f = LittleFS.open(kLogPath, FILE_READ);
        if (f && f.size() == kFileSize) {
            uint8_t hdr[kHeaderSize] = {0};
            if (f.read(hdr, sizeof(hdr)) == static_cast<int>(sizeof(hdr))) {
                const uint16_t storedCrc = getU16(hdr + 12);
                if (logCrc16(hdr, 12) == storedCrc && getU32(hdr) == kMagic &&
                    getU16(hdr + 4) == kStoreVersion && getU16(hdr + 6) == kFlashCapacity) {
                    head_ = getU16(hdr + 8);
                    count_ = getU16(hdr + 10);
                    if (head_ < kFlashCapacity && count_ <= kFlashCapacity) {
                        f.close();
                        ready_ = true;
                        return true;
                    }
                }
            }
        }
        if (f) {
            f.close();
        }
    }

    // (Re)create a zero-filled ring file of the full fixed size.
    File f = LittleFS.open(kLogPath, FILE_WRITE);
    if (!f) {
        return false;
    }
    uint8_t zeros[64] = {0};
    uint32_t remaining = kFileSize;
    while (remaining > 0) {
        const size_t chunk = remaining < sizeof(zeros) ? remaining : sizeof(zeros);
        if (f.write(zeros, chunk) != chunk) {
            f.close();
            return false;
        }
        remaining -= chunk;
    }
    f.close();

    head_ = 0;
    count_ = 0;
    ready_ = true;
    return writeHeader();
}

bool LogStore::writeHeader() {
    if (!ready_) {
        return false;
    }
    uint8_t hdr[kHeaderSize] = {0};
    putU32(hdr + 0, kMagic);
    putU16(hdr + 4, kStoreVersion);
    putU16(hdr + 6, kFlashCapacity);
    putU16(hdr + 8, head_);
    putU16(hdr + 10, count_);
    putU16(hdr + 12, logCrc16(hdr, 12));

    File f = LittleFS.open(kLogPath, "r+");
    if (!f) {
        return false;
    }
    f.seek(0);
    const size_t written = f.write(hdr, sizeof(hdr));
    f.close();
    return written == sizeof(hdr);
}

bool LogStore::append(const LogRecord& r) {
    if (!ready_) {
        return false;
    }
    uint8_t wire[kRecordWireSize] = {0};
    if (encodeRecord(r, wire, sizeof(wire)) != kRecordWireSize) {
        return false;
    }

    File f = LittleFS.open(kLogPath, "r+");
    if (!f) {
        return false;
    }
    f.seek(slotOffset(head_));
    const size_t written = f.write(wire, sizeof(wire));
    f.close();
    if (written != sizeof(wire)) {
        return false;
    }

    head_ = static_cast<uint16_t>((head_ + 1) % kFlashCapacity);
    if (count_ < kFlashCapacity) {
        ++count_;
    }
    return writeHeader();
}

uint16_t LogStore::readRecent(LogRecord* out, uint16_t max) const {
    if (!ready_ || out == nullptr || max == 0 || count_ == 0) {
        return 0;
    }
    const uint16_t n = max < count_ ? max : count_;
    // Oldest of the n most-recent records.
    const uint16_t start = static_cast<uint16_t>((head_ + kFlashCapacity - n) % kFlashCapacity);

    File f = LittleFS.open(kLogPath, FILE_READ);
    if (!f) {
        return 0;
    }
    uint16_t produced = 0;
    for (uint16_t i = 0; i < n; ++i) {
        const uint16_t slot = static_cast<uint16_t>((start + i) % kFlashCapacity);
        uint8_t wire[kRecordWireSize] = {0};
        f.seek(slotOffset(slot));
        if (f.read(wire, sizeof(wire)) != static_cast<int>(sizeof(wire))) {
            break;
        }
        LogRecord rec;
        if (decodeRecord(wire, sizeof(wire), rec)) {
            out[produced++] = rec;
        }
    }
    f.close();
    return produced;
}

bool LogStore::clear() {
    if (!ready_) {
        return false;
    }
    head_ = 0;
    count_ = 0;
    return writeHeader();
}

}  // namespace logging

#endif  // FT_FEATURE_SD_LOG
