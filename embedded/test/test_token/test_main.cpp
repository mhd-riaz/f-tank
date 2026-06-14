#include <unity.h>

#include <cstring>

#include "api/TokenGenerator.h"

using api::isTokenChar;
using api::isUrlSafeToken;
using api::kTokenAlphabet;
using api::kTokenAlphabetSize;
using api::tokenCharFromRandom;

namespace {

void test_alphabet_is_64_url_safe_chars() {
    TEST_ASSERT_EQUAL_UINT8(64, kTokenAlphabetSize);
    for (uint8_t i = 0; i < kTokenAlphabetSize; ++i) {
        TEST_ASSERT_TRUE(isTokenChar(kTokenAlphabet[i]));
    }
}

void test_token_char_always_in_alphabet() {
    // Every possible modulo bucket maps to a valid alphabet character, and the
    // mapping wraps cleanly past the alphabet size.
    for (uint32_t r = 0; r < 200; ++r) {
        const char c = tokenCharFromRandom(r);
        TEST_ASSERT_TRUE(isTokenChar(c));
        TEST_ASSERT_EQUAL_CHAR(kTokenAlphabet[r % kTokenAlphabetSize], c);
    }
    // Large values (near 32-bit max) must also map safely.
    TEST_ASSERT_TRUE(isTokenChar(tokenCharFromRandom(0xFFFFFFFFu)));
    TEST_ASSERT_TRUE(isTokenChar(tokenCharFromRandom(0x80000000u)));
}

void test_token_char_covers_whole_alphabet() {
    // Inputs 0..size-1 enumerate the full alphabet in order.
    for (uint8_t i = 0; i < kTokenAlphabetSize; ++i) {
        TEST_ASSERT_EQUAL_CHAR(kTokenAlphabet[i], tokenCharFromRandom(i));
    }
}

void test_is_token_char_rejects_non_url_safe() {
    TEST_ASSERT_TRUE(isTokenChar('A'));
    TEST_ASSERT_TRUE(isTokenChar('z'));
    TEST_ASSERT_TRUE(isTokenChar('0'));
    TEST_ASSERT_TRUE(isTokenChar('-'));
    TEST_ASSERT_TRUE(isTokenChar('_'));
    TEST_ASSERT_FALSE(isTokenChar('/'));
    TEST_ASSERT_FALSE(isTokenChar('+'));
    TEST_ASSERT_FALSE(isTokenChar(' '));
    TEST_ASSERT_FALSE(isTokenChar('='));
    TEST_ASSERT_FALSE(isTokenChar('\0'));
}

void test_url_safe_token_validation() {
    TEST_ASSERT_TRUE(isUrlSafeToken("abcDEF123-_", 11));
    TEST_ASSERT_FALSE(isUrlSafeToken("abcDEF123-_", 10));  // length mismatch (too long)
    TEST_ASSERT_FALSE(isUrlSafeToken("abc", 5));           // shorter than expected
    TEST_ASSERT_FALSE(isUrlSafeToken("bad/char", 8));      // contains '/'
    TEST_ASSERT_FALSE(isUrlSafeToken(nullptr, 4));
}

void test_url_safe_token_empty() {
    TEST_ASSERT_TRUE(isUrlSafeToken("", 0));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_alphabet_is_64_url_safe_chars);
    RUN_TEST(test_token_char_always_in_alphabet);
    RUN_TEST(test_token_char_covers_whole_alphabet);
    RUN_TEST(test_is_token_char_rejects_non_url_safe);
    RUN_TEST(test_url_safe_token_validation);
    RUN_TEST(test_url_safe_token_empty);
    return UNITY_END();
}
