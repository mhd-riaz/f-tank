#include "api/TokenGenerator.h"

#include <esp_random.h>

namespace api {

bool generateToken(char* out, size_t outSize) {
    if (out == nullptr || outSize < 2) {
        return false;
    }
    const size_t len = outSize - 1;
    for (size_t i = 0; i < len; ++i) {
        // Rejection-free modulo bias is negligible for a 64-char alphabet over
        // 32-bit randomness; acceptable for a device-pairing token. Mapping is
        // the pure tokenCharFromRandom() helper (unit-tested natively).
        out[i] = tokenCharFromRandom(esp_random());
    }
    out[len] = '\0';
    return true;
}

}  // namespace api
