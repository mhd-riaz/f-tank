#ifndef CONFIG_FEATURES_H
#define CONFIG_FEATURES_H

/**
 * @file features.h
 * @brief Compile-time feature flags for SKU/flash-tier builds.
 *
 * Flags are set per PlatformIO environment via -D build_flags (see
 * platformio.ini). This header only provides safe defaults so any translation
 * unit compiles even if a flag is undefined. Disabled features are NOT linked,
 * shrinking the image for cost-optimized 4 MB SKUs.
 *
 * SKU matrix:
 *   4 MB "Basic"  : scheduling, channels, temp, alerts, display, BLE, local API,
 *                   signed OTA. Cloud OFF, persistent logging OFF (on-demand
 *                   stream over the local API only).
 *   8 MB "Full"   : everything above + cloud account/remote + SD/local logging.
 *
 * OTA is intentionally enabled on BOTH tiers — a deployable fleet must remain
 * security-patchable.
 */

// Cloud account + remote access (TLS to backend). Heavy (mbedTLS). 8 MB only.
#ifndef FT_FEATURE_CLOUD
#define FT_FEATURE_CLOUD 0
#endif

// Persistent local logging to a filesystem / SD card (Premium). 8 MB only.
#ifndef FT_FEATURE_SD_LOG
#define FT_FEATURE_SD_LOG 0
#endif

// Signed OTA with rollback. Enabled on all SKUs by default.
#ifndef FT_FEATURE_OTA
#define FT_FEATURE_OTA 1
#endif

// Convenience: true when any persistent log sink exists. On-demand API log
// streaming does not require this and is always available.
#define FT_HAS_PERSISTENT_LOG (FT_FEATURE_SD_LOG)

#endif  // CONFIG_FEATURES_H
