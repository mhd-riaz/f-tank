#include "storage/ConfigStore.h"

#include <Preferences.h>

#include "storage/FactoryDefaults.h"

namespace storage {
namespace {

constexpr char kNamespace[] = "ftank";
constexpr char kBlobKey[] = "cfg";

}  // namespace

LoadResult ConfigStore::begin() {
    Preferences prefs;
    nvsReady_ = prefs.begin(kNamespace, /*readOnly=*/false);

    uint8_t blob[kBlobSize] = {0};
    size_t readLen = 0;
    if (nvsReady_) {
        readLen = prefs.getBytes(kBlobKey, blob, sizeof(blob));
    }
    prefs.end();

    if (readLen == 0) {
        seedFactoryDefaults(config_);
        writeBlob(config_);
        return LoadResult::kSeededFirstRun;
    }

    if (unpack(blob, readLen, config_)) {
        lastSavedCrc_ = crc32(reinterpret_cast<const uint8_t*>(&config_), sizeof(config_));
        return LoadResult::kLoaded;
    }

    // Corrupt or version mismatch -> discard and re-seed.
    seedFactoryDefaults(config_);
    writeBlob(config_);
    return LoadResult::kSeededInvalid;
}

bool ConfigStore::save() {
    const uint32_t crc = crc32(reinterpret_cast<const uint8_t*>(&config_), sizeof(config_));
    if (crc == lastSavedCrc_) {
        return false;  // unchanged -> skip write (wear-leveling)
    }
    return writeBlob(config_);
}

bool ConfigStore::reset() {
    seedFactoryDefaults(config_);
    return writeBlob(config_);
}

bool ConfigStore::writeBlob(const PersistentConfig& cfg) {
    uint8_t blob[kBlobSize] = {0};
    if (pack(cfg, blob, sizeof(blob)) != kBlobSize) {
        return false;
    }

    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    const size_t written = prefs.putBytes(kBlobKey, blob, sizeof(blob));
    prefs.end();

    if (written == sizeof(blob)) {
        lastSavedCrc_ = crc32(reinterpret_cast<const uint8_t*>(&cfg), sizeof(cfg));
        return true;
    }
    return false;
}

}  // namespace storage
