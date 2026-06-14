#ifndef PROVISIONING_PROVISIONING_SESSION_H
#define PROVISIONING_PROVISIONING_SESSION_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "api/ApiValidation.h"
#include "network/NetworkStore.h"
#include "storage/PersistentConfig.h"

/**
 * @file ProvisioningSession.h
 * @brief Hardware-free accumulator + validator for BLE provisioning input.
 *
 * Collects SSID / password / timezone written over BLE characteristics and
 * validates them before anything is persisted (NFR-8). Decoupled from NimBLE so
 * the state logic is unit-tested natively. The BLE layer feeds setters; the app
 * layer reads the validated result and commits it to NVS + ConfigBroker.
 */
namespace provisioning {

enum class SessionState : uint8_t {
    kIdle,        ///< nothing written yet
    kCollecting,  ///< partial fields received
    kReady,       ///< all required fields present + valid
    kInvalid,     ///< a field failed validation
};

class ProvisioningSession {
  public:
    void reset() {
        ssid_[0] = password_[0] = '\0';
        snprintf(tz_, sizeof(tz_), "%s", "UTC0");
        haveSsid_ = havePassword_ = false;
        state_ = SessionState::kIdle;
    }

    /// Returns false (and marks invalid) if the value is out of bounds.
    bool setSsid(const char* value) {
        if (value == nullptr) {
            state_ = SessionState::kInvalid;
            return false;
        }
        const size_t len = strlen(value);
        if (len == 0 || len >= sizeof(ssid_)) {
            state_ = SessionState::kInvalid;
            return false;
        }
        snprintf(ssid_, sizeof(ssid_), "%s", value);
        haveSsid_ = true;
        recompute();
        return true;
    }

    bool setPassword(const char* value) {
        if (value == nullptr || strlen(value) >= sizeof(password_)) {
            state_ = SessionState::kInvalid;
            return false;
        }
        snprintf(password_, sizeof(password_), "%s", value);
        havePassword_ = true;
        recompute();
        return true;
    }

    bool setTz(const char* value) {
        if (!api::isValidTz(value)) {
            state_ = SessionState::kInvalid;
            return false;
        }
        snprintf(tz_, sizeof(tz_), "%s", value);
        recompute();
        return true;
    }

    SessionState state() const {
        return state_;
    }
    bool isReady() const {
        return state_ == SessionState::kReady;
    }

    const char* ssid() const {
        return ssid_;
    }
    const char* password() const {
        return password_;
    }
    const char* tz() const {
        return tz_;
    }

  private:
    void recompute() {
        if (state_ == SessionState::kInvalid) {
            return;
        }
        // SSID is the only strictly required field; password may be empty (open
        // network) but must have been explicitly written.
        if (haveSsid_ && havePassword_) {
            state_ = SessionState::kReady;
        } else {
            state_ = SessionState::kCollecting;
        }
    }

    char ssid_[network::kSsidSize] = {0};
    char password_[network::kPasswordSize] = {0};
    char tz_[storage::kTzStringSize] = "UTC0";
    bool haveSsid_ = false;
    bool havePassword_ = false;  ///< password may be empty (open net) once written
    SessionState state_ = SessionState::kIdle;
};

}  // namespace provisioning

#endif  // PROVISIONING_PROVISIONING_SESSION_H
