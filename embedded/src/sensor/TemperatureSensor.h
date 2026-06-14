#ifndef SENSOR_TEMPERATURE_SENSOR_H
#define SENSOR_TEMPERATURE_SENSOR_H

#include <OneWire.h>

#include "sensor/Ds18b20Math.h"

/**
 * @file TemperatureSensor.h
 * @brief Non-blocking DS18B20 driver with cached address and CRC validation.
 *
 * Caches the sensor ROM address once (FR-18), runs the 750 ms conversion
 * without blocking (start now, read on a later cycle — FR-20), CRC-checks every
 * scratchpad read and rejects bad data (FR-19), and reports a sensor-fault
 * after repeated failures (FR-21). Self-heals if the probe is reconnected
 * (NFR-4).
 */
namespace sensor {

enum class TempStatus : uint8_t {
    kInit,      ///< not yet read
    kOk,        ///< last reading valid
    kNoSensor,  ///< no probe found on the bus
    kCrcFail,   ///< last scratchpad failed CRC
    kFault,     ///< repeated failures -> persistent fault (alert)
};

class TemperatureSensor {
  public:
    explicit TemperatureSensor(uint8_t oneWirePin) : oneWire_(oneWirePin) {}

    /// Locate and cache the sensor address. Safe to call again to re-probe.
    void begin();

    /// Cooperative servicing; non-blocking. Call frequently from loop().
    void update(uint32_t nowMs);

    /// Last valid temperature in Celsius, or kFaultTempC if never valid.
    float celsius() const {
        return lastValidCelsius_;
    }

    TempStatus status() const {
        return status_;
    }

    /// True when the most recent reading was valid.
    bool hasValidReading() const {
        return status_ == TempStatus::kOk;
    }

    /// True when a persistent fault is active (drives alert + heater cutoff).
    bool inFault() const {
        return status_ == TempStatus::kFault;
    }

  private:
    enum class Phase : uint8_t { kIdle, kConverting };

    bool findSensor();
    void startConversion();
    void readConversion();
    void registerFailure();

    OneWire oneWire_;
    uint8_t address_[8] = {0};
    bool addressValid_ = false;

    Phase phase_ = Phase::kIdle;
    TempStatus status_ = TempStatus::kInit;
    float lastValidCelsius_ = kFaultTempC;
    uint8_t consecutiveFailures_ = 0;

    uint32_t lastCycleMs_ = 0;
    uint32_t convStartMs_ = 0;
};

}  // namespace sensor

#endif  // SENSOR_TEMPERATURE_SENSOR_H
