#ifndef CONFIG_SECRETS_EXAMPLE_H
#define CONFIG_SECRETS_EXAMPLE_H

/**
 * @file secrets.example.h
 * @brief Template for build-time secrets. Copy to `secrets.h` (git-ignored).
 *
 * Production secrets (WiFi creds, device tokens, cloud keys) MUST live in NVS,
 * not in source (NFR-5). This header is only for local developer builds / CI
 * placeholders. NEVER commit a real `secrets.h`.
 */

// Optional developer-only WiFi fallback (leave empty for BLE provisioning).
#define FT_DEV_WIFI_SSID ""
#define FT_DEV_WIFI_PASSWORD ""

// Optional OTA public-key fingerprint placeholder (real key provisioned later).
#define FT_OTA_PUBKEY_FINGERPRINT ""

#endif  // CONFIG_SECRETS_EXAMPLE_H
