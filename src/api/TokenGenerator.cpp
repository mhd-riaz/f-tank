#include "api/TokenGenerator.h"

#include <esp_random.h>

namespace api {
namespace {

// URL-safe alphabet (no padding, no ambiguous chars handling needed here).
constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
constexpr uint8_t kAlphabetSize = sizeof(kAlphabet) - 1;

}  // namespace

bool generateToken(char* out, size_t outSize) {
    if (out == nullptr || outSize < 2) {
        return false;
    }
    const size_t len = outSize - 1;
    for (size_t i = 0; i < len; ++i) {
        // Rejection-free modulo bias is negligible for a 64-char alphabet over
        // 32-bit randomness; acceptable for a device-pairing token.
        out[i] = kAlphabet[esp_random() % kAlphabetSize];
    }
    out[len] = '\0';
    return true;
}

}  // namespace api
