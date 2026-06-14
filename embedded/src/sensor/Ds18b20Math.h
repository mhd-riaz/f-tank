#ifndef SENSOR_DS18B20_MATH_H
#define SENSOR_DS18B20_MATH_H

#include <cstddef>
#include <cstdint>

/**
 * @file Ds18b20Math.h
 * @brief Pure DS18B20 helpers (CRC + raw->Celsius), hardware-free & testable.
 */
namespace sensor {

/// Sentinel returned for an invalid/disconnected sensor (FR-19).
constexpr float kFaultTempC = -127.0f;

/// Dallas/Maxim 1-Wire CRC-8 (polynomial 0x31, reflected). 0 == data intact.
inline uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        uint8_t inbyte = data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

/// Convert the raw 16-bit two's-complement reading to Celsius (12-bit = 1/16°C).
inline float rawToCelsius(int16_t raw) {
    return static_cast<float>(raw) * 0.0625f;
}

/// Assemble the raw reading from scratchpad bytes 0 (LSB) and 1 (MSB).
inline int16_t rawFromScratchpad(uint8_t lsb, uint8_t msb) {
    return static_cast<int16_t>((static_cast<uint16_t>(msb) << 8) | lsb);
}

/// True if temperature is physically plausible for an aquarium probe.
inline bool isPlausibleCelsius(float c) {
    return c > -55.0f && c < 125.0f;
}

/// Defense-in-depth aquarium-water band. Tighter than the DS18B20 electrical
/// range above: it rejects readings that are valid for the chip but impossible
/// for a real tank — most importantly the sensor's 85.0 C power-on-reset value,
/// which passes isPlausibleCelsius(). Bounds bracket the full configurable alert
/// range (-10..60 C, see api::isValidThresholds) plus headroom so a legitimately
/// configured high reading is never discarded.
constexpr float kAquariumMinC = -10.0f;
constexpr float kAquariumMaxC = 70.0f;

inline bool isPlausibleAquariumCelsius(float c) {
    return c >= kAquariumMinC && c <= kAquariumMaxC;
}

}  // namespace sensor

#endif  // SENSOR_DS18B20_MATH_H
