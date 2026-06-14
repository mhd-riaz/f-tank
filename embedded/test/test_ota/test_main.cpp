#include <unity.h>

#include <cstring>

#include "ota/FirmwareVersion.h"
#include "ota/OtaTypes.h"

using ota::compareVersion;
using ota::FirmwareVersion;
using ota::hexDecode;
using ota::isImageSizeValid;
using ota::isNewer;
using ota::parseVersion;

namespace {

void test_parse_version() {
    FirmwareVersion v;
    TEST_ASSERT_TRUE(parseVersion("2.1.5", v));
    TEST_ASSERT_EQUAL_UINT16(2, v.major);
    TEST_ASSERT_EQUAL_UINT16(1, v.minor);
    TEST_ASSERT_EQUAL_UINT16(5, v.patch);
}

void test_parse_version_with_suffix() {
    FirmwareVersion v;
    TEST_ASSERT_TRUE(parseVersion("2.0.0-dev", v));
    TEST_ASSERT_EQUAL_UINT16(2, v.major);
    TEST_ASSERT_EQUAL_UINT16(0, v.minor);
    TEST_ASSERT_EQUAL_UINT16(0, v.patch);
}

void test_parse_version_rejects_garbage() {
    FirmwareVersion v;
    TEST_ASSERT_FALSE(parseVersion("not-a-version", v));
    TEST_ASSERT_FALSE(parseVersion("1.2", v));
    TEST_ASSERT_FALSE(parseVersion(nullptr, v));
}

void test_compare_version() {
    FirmwareVersion a{2, 1, 0};
    FirmwareVersion b{2, 0, 9};
    TEST_ASSERT_TRUE(compareVersion(a, b) > 0);
    TEST_ASSERT_TRUE(compareVersion(b, a) < 0);
    TEST_ASSERT_EQUAL_INT(0, compareVersion(a, a));
}

void test_is_newer_anti_rollback() {
    FirmwareVersion current{2, 0, 0};
    TEST_ASSERT_TRUE(isNewer(FirmwareVersion{2, 0, 1}, current));
    TEST_ASSERT_TRUE(isNewer(FirmwareVersion{3, 0, 0}, current));
    TEST_ASSERT_FALSE(isNewer(FirmwareVersion{2, 0, 0}, current));  // equal rejected
    TEST_ASSERT_FALSE(isNewer(FirmwareVersion{1, 9, 9}, current));  // older rejected
}

void test_hex_decode_valid() {
    uint8_t out[4] = {0};
    TEST_ASSERT_TRUE(hexDecode("deadBEEF", out, 4));
    TEST_ASSERT_EQUAL_HEX8(0xDE, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBE, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0xEF, out[3]);
}

void test_hex_decode_rejects_bad() {
    uint8_t out[4] = {0};
    TEST_ASSERT_FALSE(hexDecode("dead", out, 4));      // wrong length
    TEST_ASSERT_FALSE(hexDecode("deadbeeg", out, 4));  // 'g' not hex
    TEST_ASSERT_FALSE(hexDecode(nullptr, out, 4));
}

void test_hex_decode_sha256_length() {
    const char* sha = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    uint8_t out[ota::kSha256Len] = {0};
    TEST_ASSERT_TRUE(hexDecode(sha, out, ota::kSha256Len));
    TEST_ASSERT_EQUAL_HEX8(0xe3, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x55, out[ota::kSha256Len - 1]);
}

void test_image_size_validation() {
    TEST_ASSERT_TRUE(isImageSizeValid(1000, 0x300000));
    TEST_ASSERT_FALSE(isImageSizeValid(0, 0x300000));         // empty
    TEST_ASSERT_FALSE(isImageSizeValid(0x300001, 0x300000));  // too big
    TEST_ASSERT_TRUE(isImageSizeValid(0x300000, 0x300000));   // exact fit
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_version);
    RUN_TEST(test_parse_version_with_suffix);
    RUN_TEST(test_parse_version_rejects_garbage);
    RUN_TEST(test_compare_version);
    RUN_TEST(test_is_newer_anti_rollback);
    RUN_TEST(test_hex_decode_valid);
    RUN_TEST(test_hex_decode_rejects_bad);
    RUN_TEST(test_hex_decode_sha256_length);
    RUN_TEST(test_image_size_validation);
    return UNITY_END();
}
