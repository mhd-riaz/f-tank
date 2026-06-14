#ifndef API_AUTH_TOKEN_H
#define API_AUTH_TOKEN_H

#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 * @file AuthToken.h
 * @brief Pure bearer-token helpers: constant-time compare + header parsing.
 *
 * Hardware-free so it is unit-tested natively. Constant-time comparison avoids
 * leaking token length/content via timing (OWASP / CWE-208).
 */
namespace api {

/// Compare two NUL-terminated tokens in constant time. Returns true if equal.
/// Time depends only on `maxLen`, not on where the strings differ.
inline bool constantTimeEquals(const char* a, const char* b, size_t maxLen) {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    uint8_t diff = 0;
    size_t i = 0;
    for (; i < maxLen; ++i) {
        const uint8_t ca = static_cast<uint8_t>(a[i]);
        const uint8_t cb = static_cast<uint8_t>(b[i]);
        diff |= static_cast<uint8_t>(ca ^ cb);
        if (ca == 0 && cb == 0) {
            break;
        }
    }
    return diff == 0;
}

/**
 * @brief Extract the token from an "Authorization: Bearer <token>" header.
 * @param header     full header value (may be null)
 * @param outToken   destination buffer
 * @param outSize    size of destination buffer
 * @return true if a non-empty bearer token was extracted (and fit).
 */
inline bool extractBearerToken(const char* header, char* outToken, size_t outSize) {
    if (header == nullptr || outToken == nullptr || outSize == 0) {
        return false;
    }
    static const char kPrefix[] = "Bearer ";
    const size_t prefixLen = sizeof(kPrefix) - 1;
    if (strncmp(header, kPrefix, prefixLen) != 0) {
        return false;
    }
    const char* token = header + prefixLen;
    while (*token == ' ') {
        ++token;
    }
    const size_t len = strlen(token);
    if (len == 0 || len >= outSize) {
        return false;  // empty or would overflow
    }
    memcpy(outToken, token, len + 1);
    return true;
}

}  // namespace api

#endif  // API_AUTH_TOKEN_H
