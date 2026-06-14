#include "sensor/TemperatureSensor.h"

namespace sensor {
namespace {

constexpr uint8_t kCmdConvertT = 0x44;
constexpr uint8_t kCmdReadScratchpad = 0xBE;
constexpr uint32_t kConversionMs = 750;        // 12-bit conversion time
constexpr uint32_t kReadIntervalMs = 5000;     // read every ~5 s (FR-20)
constexpr uint32_t kReprobeIntervalMs = 5000;  // retry search when absent
constexpr uint8_t kFaultThreshold = 3;         // consecutive fails -> fault (FR-21)
constexpr uint8_t kScratchpadLen = 9;

}  // namespace

void TemperatureSensor::begin() {
    addressValid_ = findSensor();
    status_ = addressValid_ ? TempStatus::kInit : TempStatus::kNoSensor;
    phase_ = Phase::kIdle;
}

bool TemperatureSensor::findSensor() {
    oneWire_.reset_search();
    uint8_t addr[8] = {0};
    if (!oneWire_.search(addr)) {
        oneWire_.reset_search();
        return false;
    }
    // Validate ROM CRC (byte 7) before trusting the address.
    if (crc8(addr, 7) != addr[7]) {
        return false;
    }
    for (uint8_t i = 0; i < 8; ++i) {
        address_[i] = addr[i];
    }
    return true;
}

void TemperatureSensor::update(uint32_t nowMs) {
    if (!addressValid_) {
        if (nowMs - lastCycleMs_ >= kReprobeIntervalMs) {
            lastCycleMs_ = nowMs;
            addressValid_ = findSensor();
            if (addressValid_) {
                status_ = TempStatus::kInit;  // recovered; will read next cycle
            }
        }
        return;
    }

    switch (phase_) {
        case Phase::kIdle:
            if (nowMs - lastCycleMs_ >= kReadIntervalMs || status_ == TempStatus::kInit) {
                startConversion();
                convStartMs_ = nowMs;
                phase_ = Phase::kConverting;
            }
            break;
        case Phase::kConverting:
            if (nowMs - convStartMs_ >= kConversionMs) {
                readConversion();
                lastCycleMs_ = nowMs;
                phase_ = Phase::kIdle;
            }
            break;
    }
}

void TemperatureSensor::startConversion() {
    oneWire_.reset();
    oneWire_.select(address_);
    oneWire_.write(kCmdConvertT, 1);  // parasite-power friendly
}

void TemperatureSensor::readConversion() {
    if (oneWire_.reset() == 0) {  // no presence pulse -> probe gone
        registerFailure();
        return;
    }
    oneWire_.select(address_);
    oneWire_.write(kCmdReadScratchpad);

    uint8_t data[kScratchpadLen] = {0};
    for (uint8_t i = 0; i < kScratchpadLen; ++i) {
        data[i] = oneWire_.read();
    }

    if (crc8(data, 8) != data[8]) {
        status_ = TempStatus::kCrcFail;
        registerFailure();
        return;
    }

    const float celsius = rawToCelsius(rawFromScratchpad(data[0], data[1]));
    if (!isPlausibleCelsius(celsius)) {
        registerFailure();
        return;
    }

    lastValidCelsius_ = celsius;
    status_ = TempStatus::kOk;
    consecutiveFailures_ = 0;
}

void TemperatureSensor::registerFailure() {
    if (consecutiveFailures_ < kFaultThreshold) {
        ++consecutiveFailures_;
    }
    if (consecutiveFailures_ >= kFaultThreshold) {
        status_ = TempStatus::kFault;
        lastValidCelsius_ = kFaultTempC;
    }
}

}  // namespace sensor
