#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

#include <cstdint>

/**
 * @file pins.h
 * @brief Centralized physical pin map for the f-tank controller (ESP32 DevKit).
 *
 * WARNING: These are PROVISIONAL ESP32-safe GPIOs and MUST be confirmed against
 * the production PCB before release. Do NOT reuse the legacy v1 pins (4-11):
 * GPIO 6-11 are bonded to SPI flash on the ESP32 and will break boot.
 * GPIO 34-39 are input-only and cannot drive relays/buzzer.
 */

namespace pins {

// I2C bus — RTC (DS3231) + OLED (SSD1306).
constexpr uint8_t kI2cSda = 21;
constexpr uint8_t kI2cScl = 22;

// OneWire bus — DS18B20 temperature sensor.
constexpr uint8_t kOneWire = 16;

// Buzzer / shared alert output.
constexpr uint8_t kBuzzer = 13;

// Generic relay channel outputs. Order defines default channel index.
// Confirm count/assignment against the production PCB.
constexpr uint8_t kRelayChannels[] = {26, 25, 33, 32};
constexpr uint8_t kRelayChannelCount =
    static_cast<uint8_t>(sizeof(kRelayChannels) / sizeof(kRelayChannels[0]));

}  // namespace pins

#endif  // CONFIG_PINS_H
