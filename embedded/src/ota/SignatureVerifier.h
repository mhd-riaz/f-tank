#ifndef OTA_SIGNATURE_VERIFIER_H
#define OTA_SIGNATURE_VERIFIER_H

#include <cstddef>
#include <cstdint>

#include "ota/OtaTypes.h"

/**
 * @file SignatureVerifier.h
 * @brief Verifies an ECDSA-P256 signature over a SHA-256 digest (mbedTLS).
 *
 * The embedded OTA public key (config/ota_pubkey.h) authenticates the firmware
 * image (FR-39, NFR-7). The private key stays offline in the signing pipeline.
 */
namespace ota {

class SignatureVerifier {
  public:
    /**
     * @brief Verify a DER-encoded ECDSA signature over a 32-byte SHA-256 digest.
     * @param digest    SHA-256 digest of the firmware image (kSha256Len bytes)
     * @param sig       DER signature bytes
     * @param sigLen    signature length
     * @return true only if the signature is valid for the embedded public key.
     */
    static bool verify(const uint8_t* digest, const uint8_t* sig, size_t sigLen);
};

}  // namespace ota

#endif  // OTA_SIGNATURE_VERIFIER_H
