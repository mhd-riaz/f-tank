#include "ota/SignatureVerifier.h"

#include <mbedtls/md.h>
#include <mbedtls/pk.h>

#include <cstring>

#include "config/ota_pubkey.h"

namespace ota {

bool SignatureVerifier::verify(const uint8_t* digest, const uint8_t* sig, size_t sigLen) {
    if (digest == nullptr || sig == nullptr || sigLen == 0 || sigLen > kMaxSignatureLen) {
        return false;
    }

    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    // Parse the embedded PEM public key (length includes the trailing NUL).
    int rc = mbedtls_pk_parse_public_key(&pk, reinterpret_cast<const uint8_t*>(kOtaPublicKeyPem),
                                         sizeof(kOtaPublicKeyPem));
    if (rc != 0) {
        mbedtls_pk_free(&pk);
        return false;
    }

    // Verify the ECDSA signature over the SHA-256 digest.
    rc = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, digest, kSha256Len, sig, sigLen);

    mbedtls_pk_free(&pk);
    return rc == 0;
}

}  // namespace ota
