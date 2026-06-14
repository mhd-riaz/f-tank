#include "control/SafetyController.h"

namespace control {

void SafetyController::begin() {
    cutTargetMask_ = 0;
    for (uint8_t i = 0; i < channels_.count(); ++i) {
        if (channels_.config(i).cutOnOverTemp) {
            cutTargetMask_ |= static_cast<uint16_t>(1u << i);
        }
    }
}

void SafetyController::applyCut(bool cut) {
    if (cut == cutEngaged_) {
        return;
    }
    cutEngaged_ = cut;
    for (uint8_t i = 0; i < channels_.count(); ++i) {
        if (cutTargetMask_ & (1u << i)) {
            runner_.setForceOff(i, cut);  // runner keeps it off without flicker
        }
    }
}

void SafetyController::update() {
    const bool sensorFault = temp_.inFault();
    alerts_.set(alert::AlertId::kSensorFault, sensorFault);

    if (sensorFault) {
        // Temperature can't be trusted -> fail safe: cut heater, clear temp bands.
        highActive_ = false;
        lowActive_ = false;
        alerts_.set(alert::AlertId::kHighTemp, false);
        alerts_.set(alert::AlertId::kLowTemp, false);
        applyCut(true);
        return;
    }

    if (!temp_.hasValidReading()) {
        return;  // no data yet; hold previous state
    }

    const alert::TempBand band =
        alert::evaluateBand(temp_.celsius(), thresholds_, highActive_, lowActive_);
    highActive_ = (band == alert::TempBand::kHigh);
    lowActive_ = (band == alert::TempBand::kLow);

    alerts_.set(alert::AlertId::kHighTemp, highActive_);
    alerts_.set(alert::AlertId::kLowTemp, lowActive_);

    // High temp is the dangerous case for livestock -> cut heater. Low temp
    // alerts only (cutting the heater would make a cold tank colder).
    applyCut(highActive_);
}

}  // namespace control
