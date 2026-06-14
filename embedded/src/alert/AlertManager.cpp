#include "alert/AlertManager.h"

#include <Arduino.h>

namespace alert {
namespace {

constexpr uint32_t kBeepOnMs = 120;       // short beep duration
constexpr uint32_t kBeepGapMs = 150;      // gap between beeps in a burst
constexpr uint32_t kBurstPauseMs = 2000;  // pause between repeated bursts
constexpr uint32_t kCriticalOnMs = 600;   // long beep for critical

uint8_t beepsForSeverity(Severity s) {
    switch (s) {
        case Severity::kCritical:
            return 3;  // 3 long beeps
        case Severity::kWarning:
            return 1;  // 1 short beep
        case Severity::kInfo:
        default:
            return 0;  // silent (display/app only)
    }
}

uint32_t onMsForSeverity(Severity s) {
    return s == Severity::kCritical ? kCriticalOnMs : kBeepOnMs;
}

}  // namespace

void AlertManager::begin() {
    pinMode(buzzerPin_, OUTPUT);
    digitalWrite(buzzerPin_, LOW);
}

void AlertManager::set(AlertId id, bool active) {
    const uint16_t bit = static_cast<uint16_t>(1u << static_cast<uint8_t>(id));
    if (active) {
        activeMask_ |= bit;
    } else {
        activeMask_ &= static_cast<uint16_t>(~bit);
    }
}

bool AlertManager::isActive(AlertId id) const {
    return (activeMask_ & (1u << static_cast<uint8_t>(id))) != 0;
}

Severity AlertManager::highestSeverity() const {
    Severity worst = Severity::kInfo;
    for (uint8_t i = 0; i < kAlertCount; ++i) {
        if ((activeMask_ & (1u << i)) == 0) {
            continue;
        }
        const Severity s = severityOf(static_cast<AlertId>(i));
        if (static_cast<uint8_t>(s) > static_cast<uint8_t>(worst)) {
            worst = s;
        }
    }
    return worst;
}

void AlertManager::startPattern(Severity severity, uint32_t nowMs) {
    activePattern_ = severity;
    beepsRemaining_ = beepsForSeverity(severity);
    if (beepsRemaining_ > 0) {
        buzzerOn_ = true;
        digitalWrite(buzzerPin_, HIGH);
        phaseUntilMs_ = nowMs + onMsForSeverity(severity);
    } else {
        buzzerOn_ = false;
        digitalWrite(buzzerPin_, LOW);
        phaseUntilMs_ = nowMs + kBurstPauseMs;
    }
}

void AlertManager::update(uint32_t nowMs) {
    const Severity target = highestSeverity();

    // No audible alert: ensure buzzer is off and reset pattern.
    if (target == Severity::kInfo) {
        if (buzzerOn_) {
            buzzerOn_ = false;
            digitalWrite(buzzerPin_, LOW);
        }
        beepsRemaining_ = 0;
        activePattern_ = Severity::kInfo;
        return;
    }

    // Severity changed: restart the pattern immediately.
    if (target != activePattern_ && beepsRemaining_ == 0 && !buzzerOn_) {
        startPattern(target, nowMs);
        return;
    }

    if (static_cast<int32_t>(nowMs - phaseUntilMs_) < 0) {
        return;  // still within the current phase
    }

    if (buzzerOn_) {
        // End of a beep.
        buzzerOn_ = false;
        digitalWrite(buzzerPin_, LOW);
        if (beepsRemaining_ > 0) {
            --beepsRemaining_;
        }
        phaseUntilMs_ = nowMs + (beepsRemaining_ > 0 ? kBeepGapMs : kBurstPauseMs);
    } else if (beepsRemaining_ > 0) {
        // Gap finished, more beeps to go in this burst.
        buzzerOn_ = true;
        digitalWrite(buzzerPin_, HIGH);
        phaseUntilMs_ = nowMs + onMsForSeverity(activePattern_);
    } else {
        // Burst pause finished: start a new burst at current severity.
        startPattern(target, nowMs);
    }
}

}  // namespace alert
