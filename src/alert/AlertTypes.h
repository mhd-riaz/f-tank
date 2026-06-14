#ifndef ALERT_ALERT_TYPES_H
#define ALERT_ALERT_TYPES_H

#include <cstdint>

/**
 * @file AlertTypes.h
 * @brief Alert kinds, severity, and pure temperature-band evaluation.
 *
 * Hardware-free so the threshold/hysteresis logic is unit-tested natively.
 */
namespace alert {

/// Distinct alert conditions handled by the shared alert manager (FR-26).
enum class AlertId : uint8_t {
    kHighTemp,
    kLowTemp,
    kSensorFault,
    kRtcFailure,
    kNtpFailure,
    kCount,  ///< sentinel: number of alert ids
};

constexpr uint8_t kAlertCount = static_cast<uint8_t>(AlertId::kCount);

/// Severity drives the buzzer pattern (FR-27): more/longer beeps = worse.
enum class Severity : uint8_t {
    kInfo = 0,      ///< no buzzer (display/app only)
    kWarning = 1,   ///< short beeps
    kCritical = 2,  ///< long/insistent beeps
};

/// Severity for each alert id.
inline Severity severityOf(AlertId id) {
    switch (id) {
        case AlertId::kHighTemp:
            return Severity::kCritical;  // safety: heater cut + insistent
        case AlertId::kSensorFault:
            return Severity::kCritical;  // can't verify temp -> fail safe
        case AlertId::kLowTemp:
            return Severity::kWarning;
        case AlertId::kRtcFailure:
            return Severity::kWarning;
        case AlertId::kNtpFailure:
            return Severity::kInfo;
        default:
            return Severity::kInfo;
    }
}

/// Temperature thresholds in Celsius (FR-22). User-modifiable, NVS-persisted.
struct TempThresholds {
    float lowC = 20.0f;
    float highC = 31.0f;
    float hysteresisC = 0.5f;  ///< re-entry margin to avoid flapping (FR-25)
};

/// Where the temperature sits relative to the configured band.
enum class TempBand : uint8_t { kSafe, kHigh, kLow };

/**
 * @brief Evaluate the temperature band with hysteresis.
 * @param tempC        current temperature
 * @param th           thresholds
 * @param highActive   whether a high alert is currently latched
 * @param lowActive    whether a low alert is currently latched
 *
 * A latched alert only clears once the temperature returns past the threshold
 * by `hysteresisC`, so readings hovering at the limit don't flap (FR-25).
 */
inline TempBand evaluateBand(float tempC, const TempThresholds& th, bool highActive,
                             bool lowActive) {
    if (highActive ? (tempC >= th.highC - th.hysteresisC) : (tempC >= th.highC)) {
        return TempBand::kHigh;
    }
    if (lowActive ? (tempC <= th.lowC + th.hysteresisC) : (tempC <= th.lowC)) {
        return TempBand::kLow;
    }
    return TempBand::kSafe;
}

}  // namespace alert

#endif  // ALERT_ALERT_TYPES_H
