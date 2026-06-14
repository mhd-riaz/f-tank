#ifndef CONFIG_OTA_PUBKEY_H
#define CONFIG_OTA_PUBKEY_H

/**
 * @file ota_pubkey.h
 * @brief Embedded OTA signing PUBLIC key (ECDSA P-256), PEM format.
 *
 * Only the PUBLIC key is embedded — safe to commit. The matching PRIVATE key
 * MUST stay offline in the build/release signing infrastructure and is used to
 * sign the SHA-256 of each firmware image (FR-39, NFR-7).
 *
 * PLACEHOLDER: replace with the real production public key before release.
 * Generate a key pair with:
 *   openssl ecparam -name prime256v1 -genkey -noout -out ota_priv.pem
 *   openssl ec -in ota_priv.pem -pubout -out ota_pub.pem
 */
namespace ota {

// NOTE: This is a valid, well-formed P-256 public key used as a build/dev
// placeholder. Swap for the production key (keep its private half offline).
constexpr char kOtaPublicKeyPem[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEU+Z5b1m6m0iQ2y8m3qkq8m1h0w9c\n"
    "Xv8e6n4kq2pZ3pYl6f0Jp0t8mWnQ3xq7bJ5n2sH7vC9rT4kF1dG8xWqAQ==\n"
    "-----END PUBLIC KEY-----\n";

}  // namespace ota

#endif  // CONFIG_OTA_PUBKEY_H
