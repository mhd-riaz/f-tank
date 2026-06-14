#ifndef ALERT_ALERT_MANAGER_H
#define ALERT_ALERT_MANAGER_H

#include "alert/AlertTypes.h"

/**
 * @file AlertManager.h
 * @brief Reusable alert state + non-blocking buzzer (FR-26, FR-27, FR-28).
 *
 * Tracks the active alert set, exposes it for the app/API to notify the user,
 * and drives the buzzer with a criticality-distinct pattern. Never blocks and
 * never traps the loop.
 */
namespace alert {

class AlertManager {
  public:
    explicit AlertManager(uint8_t buzzerPin) : buzzerPin_(buzzerPin) {}

    /// Configure the buzzer pin. Call once from setup().
    void begin();

    /// Set or clear an individual alert condition.
    void set(AlertId id, bool active);

    bool isActive(AlertId id) const;

    /// True if any alert is currently active (for display/app).
    bool anyActive() const {
        return activeMask_ != 0;
    }

    /// Bitmask of active alerts (bit i == AlertId i), for the API/app payload.
    uint16_t activeMask() const {
        return activeMask_;
    }

    /// Highest severity among active alerts (drives the buzzer pattern).
    Severity highestSeverity() const;

    /// Cooperative servicing; non-blocking. Call frequently from loop().
    void update(uint32_t nowMs);

  private:
    void startPattern(Severity severity, uint32_t nowMs);

    uint8_t buzzerPin_;
    uint16_t activeMask_ = 0;

    // Non-blocking buzzer pattern state.
    bool buzzerOn_ = false;
    uint8_t beepsRemaining_ = 0;
    uint32_t phaseUntilMs_ = 0;
    Severity activePattern_ = Severity::kInfo;
};

}  // namespace alert

#endif  // ALERT_ALERT_MANAGER_H
