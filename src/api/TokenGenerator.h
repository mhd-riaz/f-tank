#ifndef API_TOKEN_GENERATOR_H
#define API_TOKEN_GENERATOR_H

#include <cstddef>

/**
 * @file TokenGenerator.h
 * @brief Generates a cryptographically-random bearer token (NFR-5).
 */
namespace api {

/**
 * @brief Fill `out` with a random URL-safe token of (outSize-1) chars + NUL.
 *        Uses the ESP32 hardware RNG (esp_random). Requires outSize >= 2.
 * @return true on success.
 */
bool generateToken(char* out, size_t outSize);

}  // namespace api

#endif  // API_TOKEN_GENERATOR_H
