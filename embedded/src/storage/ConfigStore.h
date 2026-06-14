#ifndef STORAGE_CONFIG_STORE_H
#define STORAGE_CONFIG_STORE_H

#include "storage/PersistentConfig.h"

/**
 * @file ConfigStore.h
 * @brief NVS-backed persistence for the device config (FR-34).
 *
 * Loads/validates the config blob from NVS; re-seeds factory defaults on first
 * boot or on CRC/version failure. Writes only when the content actually changes
 * (flash wear-leveling per storage-optimization instruction).
 */
namespace storage {

enum class LoadResult : uint8_t {
    kLoaded,          ///< valid config read from NVS
    kSeededFirstRun,  ///< NVS empty -> factory defaults written
    kSeededInvalid,   ///< NVS corrupt/version mismatch -> defaults written
};

class ConfigStore {
  public:
    /// Open NVS and load config, seeding defaults if needed.
    LoadResult begin();

    PersistentConfig& config() {
        return config_;
    }
    const PersistentConfig& config() const {
        return config_;
    }

    /// Persist current config to NVS only if it differs from what's stored.
    /// Returns true if a write occurred.
    bool save();

    /// Restore factory defaults and persist them (factory reset, FR-10).
    /// Returns true if the defaults were written successfully.
    bool reset();

  private:
    bool writeBlob(const PersistentConfig& cfg);

    PersistentConfig config_;
    uint32_t lastSavedCrc_ = 0;  ///< CRC of the blob currently in NVS
    bool nvsReady_ = false;
};

}  // namespace storage

#endif  // STORAGE_CONFIG_STORE_H
