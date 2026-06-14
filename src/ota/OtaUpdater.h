#ifndef OTA_OTA_UPDATER_H
#define OTA_OTA_UPDATER_H

#include <esp_ota_ops.h>
#include <mbedtls/md.h>

#include "ota/FirmwareVersion.h"
#include "ota/OtaTypes.h"

/**
 * @file OtaUpdater.h
 * @brief Streaming signed OTA writer with rollback support (FR-38/39).
 *
 * Writes the uploaded image to the inactive OTA slot while hashing it; on
 * finish it checks the SHA-256 and an ECDSA signature before switching the boot
 * partition. After reboot, the new image must confirm a healthy boot or the
 * bootloader rolls back to the previous slot.
 */
namespace ota {

class OtaUpdater {
  public:
    /**
     * @brief Start an OTA session: validate metadata and open the target slot.
     * @param imageSize   declared image size in bytes
     * @param imageVer    version of the incoming image
     * @param currentVer  currently running version (anti-rollback check)
     * @param expectedSha declared SHA-256 (kSha256Len bytes)
     * @param sig         DER ECDSA signature over the SHA-256
     * @param sigLen      signature length
     */
    OtaError begin(uint32_t imageSize, const FirmwareVersion& imageVer,
                   const FirmwareVersion& currentVer, const uint8_t* expectedSha,
                   const uint8_t* sig, size_t sigLen);

    /// Write a chunk of the image. Must be called in order.
    OtaError write(const uint8_t* data, size_t len);

    /// Finalize: verify size, SHA-256, signature; set boot partition on success.
    OtaError finish();

    /// Abort an in-progress session and release resources.
    void abort();

    OtaState state() const {
        return state_;
    }
    OtaError lastError() const {
        return lastError_;
    }
    bool rebootPending() const {
        return state_ == OtaState::kReadyReboot;
    }

    // --- Rollback management (static; act on the running image) ---

    /// True if the running image still awaits a healthy-boot confirmation.
    static bool isPendingVerify();

    /// Mark the running image valid, cancelling any pending rollback.
    static void confirmRunningImageValid();

  private:
    OtaError fail(OtaError e);

    OtaState state_ = OtaState::kIdle;
    OtaError lastError_ = OtaError::kOk;
    const esp_partition_t* target_ = nullptr;
    esp_ota_handle_t handle_ = 0;
    uint32_t declaredSize_ = 0;
    uint32_t received_ = 0;
    uint8_t expectedSha_[kSha256Len] = {0};
    uint8_t signature_[kMaxSignatureLen] = {0};
    size_t signatureLen_ = 0;
    mbedtls_md_context_t sha_;
    bool shaActive_ = false;
};

}  // namespace ota

#endif  // OTA_OTA_UPDATER_H
