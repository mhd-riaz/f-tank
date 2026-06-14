#ifndef API_TOKEN_GENERATOR_H
#define API_TOKEN_GENERATOR_H

#include <cstddef>
#include <cstdint>

/**
 * @file TokenGenerator.h
 * @brief Generates a cryptographically-random bearer token (NFR-5).
 *
 * The randomness source (ESP32 hardware RNG) lives in the .cpp; the URL-safe
 * alphabet mapping + token-shape validation are pure inline helpers below so
 * they are unit-tested natively without hardware.
 */
namespace api {

/// URL-safe token alphabet (64 chars: A-Z a-z 0-9 - _). No padding/ambiguity.
constexpr char kTokenAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
constexpr uint8_t kTokenAlphabetSize = sizeof(kTokenAlphabet) - 1;

/// Map a 32-bit random value to one URL-safe alphabet character.
inline char tokenCharFromRandom(uint32_t rnd) {
    return kTokenAlphabet[rnd % kTokenAlphabetSize];
}

/// True if `c` is a member of the URL-safe token alphabet.
inline bool isTokenChar(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
           c == '_';
}

/// True if `token` is exactly `expectedLen` chars and entirely URL-safe.
inline bool isUrlSafeToken(const char* token, size_t expectedLen) {
    if (token == nullptr) {
        return false;
    }
    size_t i = 0;
    for (; token[i] != '\0'; ++i) {
        if (i >= expectedLen || !isTokenChar(token[i])) {
            return false;
        }
    }
    return i == expectedLen;
}

/**
 * @brief Fill `out` with a random URL-safe token of (outSize-1) chars + NUL.
 *        Uses the ESP32 hardware RNG (esp_random). Requires outSize >= 2.
 * @return true on success.
 */
bool generateToken(char* out, size_t outSize);

}  // namespace api

#endif  // API_TOKEN_GENERATOR_H
