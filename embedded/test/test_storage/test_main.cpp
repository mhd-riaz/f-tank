#include <unity.h>

#include <cstring>

#include "channel/ChannelController.h"
#include "storage/FactoryDefaults.h"
#include "storage/PersistentConfig.h"

using storage::kBlobSize;
using storage::kConfigVersion;
using storage::pack;
using storage::PersistentConfig;
using storage::seedFactoryDefaults;
using storage::unpack;

namespace {

void test_pack_unpack_roundtrip() {
    PersistentConfig in;
    seedFactoryDefaults(in);

    uint8_t blob[kBlobSize];
    TEST_ASSERT_EQUAL_UINT(kBlobSize, pack(in, blob, sizeof(blob)));

    PersistentConfig out;
    TEST_ASSERT_TRUE(unpack(blob, kBlobSize, out));
    TEST_ASSERT_EQUAL_UINT16(in.version, out.version);
    TEST_ASSERT_EQUAL_UINT8(in.channelCount, out.channelCount);
    TEST_ASSERT_EQUAL_INT(0, memcmp(&in, &out, sizeof(PersistentConfig)));
}

void test_unpack_rejects_bad_crc() {
    PersistentConfig in;
    seedFactoryDefaults(in);
    uint8_t blob[kBlobSize];
    pack(in, blob, sizeof(blob));

    blob[4] ^= 0xFF;  // corrupt a payload byte

    PersistentConfig out;
    TEST_ASSERT_FALSE(unpack(blob, kBlobSize, out));
}

void test_unpack_rejects_version_mismatch() {
    PersistentConfig in;
    seedFactoryDefaults(in);
    in.version = static_cast<uint16_t>(kConfigVersion + 1);  // future schema

    uint8_t blob[kBlobSize];
    pack(in, blob, sizeof(blob));

    PersistentConfig out;
    TEST_ASSERT_FALSE(unpack(blob, kBlobSize, out));  // discard + re-seed path
}

void test_unpack_rejects_wrong_size() {
    PersistentConfig in;
    seedFactoryDefaults(in);
    uint8_t blob[kBlobSize];
    pack(in, blob, sizeof(blob));

    PersistentConfig out;
    TEST_ASSERT_FALSE(unpack(blob, kBlobSize - 1, out));
}

void test_factory_defaults_match_legacy() {
    PersistentConfig cfg;
    seedFactoryDefaults(cfg);

    TEST_ASSERT_EQUAL_UINT16(kConfigVersion, cfg.version);
    TEST_ASSERT_TRUE(cfg.channelCount >= 4);

    // Heater is the cut target.
    TEST_ASSERT_TRUE(cfg.channels[2].config.cutOnOverTemp);
    // Pump uses inverted scheduling.
    TEST_ASSERT_TRUE(cfg.channels[3].schedule.inverted);
    // Light first window 10:30-14:30.
    TEST_ASSERT_EQUAL_UINT16(630, cfg.channels[0].schedule.windows[0].startMin);
    TEST_ASSERT_EQUAL_UINT16(870, cfg.channels[0].schedule.windows[0].endMin);
    // Default thresholds.
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, cfg.thresholds.lowC);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 31.0f, cfg.thresholds.highC);
}

void test_unpack_rejects_channel_count_overflow() {
    // A blob with a valid CRC + version but an out-of-range channelCount must be
    // rejected (bounds guard, NFR-8) rather than trusted and over-indexed.
    PersistentConfig in;
    seedFactoryDefaults(in);
    in.channelCount = channel::kMaxChannels + 1;  // corrupt: too many channels
    uint8_t blob[kBlobSize];
    pack(in, blob, sizeof(blob));  // CRC computed over the bad value -> still "valid"

    PersistentConfig out;
    TEST_ASSERT_FALSE(unpack(blob, kBlobSize, out));
}

void test_unpack_accepts_max_channel_count() {
    PersistentConfig in;
    seedFactoryDefaults(in);
    in.channelCount = channel::kMaxChannels;  // exactly the limit is allowed
    uint8_t blob[kBlobSize];
    pack(in, blob, sizeof(blob));

    PersistentConfig out;
    TEST_ASSERT_TRUE(unpack(blob, kBlobSize, out));
    TEST_ASSERT_EQUAL_UINT8(channel::kMaxChannels, out.channelCount);
}

void test_factory_reset_is_idempotent() {
    // ConfigStore::reset() reseeds via seedFactoryDefaults; reseeding from any
    // prior (even mutated) state must yield the identical canonical config so a
    // factory reset (FR-10) is deterministic.
    PersistentConfig a;
    seedFactoryDefaults(a);

    PersistentConfig b;
    b.channelCount = 99;  // simulate corrupted / user-modified state
    b.thresholds.lowC = -999.0f;
    seedFactoryDefaults(b);  // "reset"
    TEST_ASSERT_EQUAL_INT(0, memcmp(&a, &b, sizeof(PersistentConfig)));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_pack_unpack_roundtrip);
    RUN_TEST(test_unpack_rejects_bad_crc);
    RUN_TEST(test_unpack_rejects_version_mismatch);
    RUN_TEST(test_unpack_rejects_wrong_size);
    RUN_TEST(test_unpack_rejects_channel_count_overflow);
    RUN_TEST(test_unpack_accepts_max_channel_count);
    RUN_TEST(test_factory_defaults_match_legacy);
    RUN_TEST(test_factory_reset_is_idempotent);
    return UNITY_END();
}
