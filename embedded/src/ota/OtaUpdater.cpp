#include "ota/OtaUpdater.h"

#include <cstring>

#include "ota/SignatureVerifier.h"

namespace ota {

OtaError OtaUpdater::fail(OtaError e) {
    if (shaActive_) {
        mbedtls_md_free(&sha_);
        shaActive_ = false;
    }
    if (handle_ != 0) {
        esp_ota_abort(handle_);
        handle_ = 0;
    }
    state_ = OtaState::kFailed;
    lastError_ = e;
    return e;
}

OtaError OtaUpdater::begin(uint32_t imageSize, const FirmwareVersion& imageVer,
                           const FirmwareVersion& currentVer, const uint8_t* expectedSha,
                           const uint8_t* sig, size_t sigLen) {
    if (state_ == OtaState::kReceiving || state_ == OtaState::kVerifying) {
        return OtaError::kBusy;
    }
    if (expectedSha == nullptr || sig == nullptr || sigLen == 0 || sigLen > kMaxSignatureLen) {
        return fail(OtaError::kBadHeader);
    }
    // Anti-rollback: only accept strictly newer images (FR-39).
    if (!isNewer(imageVer, currentVer)) {
        return fail(OtaError::kNotNewer);
    }

    target_ = esp_ota_get_next_update_partition(nullptr);
    if (target_ == nullptr) {
        return fail(OtaError::kBeginFailed);
    }
    if (!isImageSizeValid(imageSize, target_->size)) {
        return fail(OtaError::kTooLarge);
    }

    if (esp_ota_begin(target_, imageSize, &handle_) != ESP_OK) {
        handle_ = 0;
        return fail(OtaError::kBeginFailed);
    }

    // Init incremental SHA-256 over the image as it streams in.
    mbedtls_md_init(&sha_);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (info == nullptr || mbedtls_md_setup(&sha_, info, 0) != 0 || mbedtls_md_starts(&sha_) != 0) {
        return fail(OtaError::kBeginFailed);
    }
    shaActive_ = true;

    memcpy(expectedSha_, expectedSha, kSha256Len);
    memcpy(signature_, sig, sigLen);
    signatureLen_ = sigLen;
    declaredSize_ = imageSize;
    received_ = 0;
    state_ = OtaState::kReceiving;
    lastError_ = OtaError::kOk;
    return OtaError::kOk;
}

OtaError OtaUpdater::write(const uint8_t* data, size_t len) {
    if (state_ != OtaState::kReceiving) {
        return OtaError::kWriteFailed;
    }
    if (data == nullptr || len == 0) {
        return OtaError::kOk;
    }
    if (received_ + len > declaredSize_) {
        return fail(OtaError::kSizeMismatch);  // more data than declared
    }
    if (esp_ota_write(handle_, data, len) != ESP_OK) {
        return fail(OtaError::kWriteFailed);
    }
    if (mbedtls_md_update(&sha_, data, len) != 0) {
        return fail(OtaError::kWriteFailed);
    }
    received_ += len;
    return OtaError::kOk;
}

OtaError OtaUpdater::finish() {
    if (state_ != OtaState::kReceiving) {
        return fail(OtaError::kWriteFailed);
    }
    state_ = OtaState::kVerifying;

    if (received_ != declaredSize_) {
        return fail(OtaError::kSizeMismatch);
    }

    uint8_t digest[kSha256Len] = {0};
    const int shaRc = mbedtls_md_finish(&sha_, digest);
    mbedtls_md_free(&sha_);
    shaActive_ = false;
    if (shaRc != 0) {
        return fail(OtaError::kHashMismatch);
    }

    // Integrity: computed hash must equal the declared hash.
    if (memcmp(digest, expectedSha_, kSha256Len) != 0) {
        return fail(OtaError::kHashMismatch);
    }

    // Authenticity: signature over the hash must verify against the OTA key.
    if (!SignatureVerifier::verify(digest, signature_, signatureLen_)) {
        return fail(OtaError::kBadSignature);
    }

    // Finalize the partition (also runs the ESP-IDF image checks).
    if (esp_ota_end(handle_) != ESP_OK) {
        handle_ = 0;
        return fail(OtaError::kActivateFailed);
    }
    handle_ = 0;

    if (esp_ota_set_boot_partition(target_) != ESP_OK) {
        return fail(OtaError::kActivateFailed);
    }

    state_ = OtaState::kReadyReboot;
    lastError_ = OtaError::kOk;
    return OtaError::kOk;
}

void OtaUpdater::abort() {
    fail(OtaError::kWriteFailed);
}

bool OtaUpdater::isPendingVerify() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t imgState;
    if (running == nullptr || esp_ota_get_state_partition(running, &imgState) != ESP_OK) {
        return false;
    }
    return imgState == ESP_OTA_IMG_PENDING_VERIFY;
}

void OtaUpdater::confirmRunningImageValid() {
    if (isPendingVerify()) {
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

}  // namespace ota
