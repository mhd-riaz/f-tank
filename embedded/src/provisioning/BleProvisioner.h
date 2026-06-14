#ifndef PROVISIONING_BLE_PROVISIONER_H
#define PROVISIONING_BLE_PROVISIONER_H

#include <cstdint>

#include "network/NetworkStore.h"
#include "provisioning/ProvisioningSession.h"
#include "storage/ConfigStore.h"

/**
 * @file BleProvisioner.h
 * @brief NimBLE GATT provisioning: WiFi creds + timezone + device token (AD-6).
 *
 * Advertises ONLY while unprovisioned (or after an explicit factory reset) to
 * minimize attack surface and free RAM in normal operation. Characteristics
 * require an encrypted (bonded) link (NFR-5/8). On apply it persists creds + TZ,
 * issues a device bearer token to the app, and signals the main loop to start
 * WiFi. BLE is torn down once provisioning succeeds.
 *
 * NimBLE chosen over Bluedroid for its much smaller flash/RAM footprint.
 */
namespace provisioning {

enum class ProvStatus : uint8_t {
    kIdle = 0,
    kAdvertising = 1,
    kConnected = 2,
    kApplied = 3,
    kError = 4,
};

class BleProvisioner {
  public:
    BleProvisioner(network::NetworkStore& netStore, storage::ConfigStore& configStore)
        : netStore_(netStore), configStore_(configStore) {}

    /// Start advertising if the device is unprovisioned. No-op otherwise.
    void beginIfUnprovisioned();

    /// Force (re)start provisioning regardless of state (factory reset path).
    void start();

    /// Cooperative servicing; non-blocking. Tears BLE down after apply.
    void update(uint32_t nowMs);

    /// True after the app applied valid credentials (main starts WiFi).
    bool justProvisioned() const {
        return justProvisioned_;
    }
    void clearProvisionedFlag() {
        justProvisioned_ = false;
    }

    bool isActive() const {
        return active_;
    }

    // --- Called from BLE callbacks (internal use) ---
    void onSsidWrite(const char* value);
    void onPasswordWrite(const char* value);
    void onTzWrite(const char* value);
    void onApplyCommand();
    void onClientConnected();
    void onClientDisconnected();

  private:
    void setupGatt();
    void teardown();
    void commit();

    network::NetworkStore& netStore_;
    storage::ConfigStore& configStore_;
    ProvisioningSession session_;

    bool active_ = false;
    bool justProvisioned_ = false;
    bool applyRequested_ = false;
    bool teardownPending_ = false;
    uint32_t teardownAtMs_ = 0;
    ProvStatus status_ = ProvStatus::kIdle;
};

}  // namespace provisioning

#endif  // PROVISIONING_BLE_PROVISIONER_H
