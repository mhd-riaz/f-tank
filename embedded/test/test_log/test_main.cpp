#include <unity.h>

#include <cstring>

#include "log/LogTypes.h"

using logging::decodeRecord;
using logging::encodeRecord;
using logging::kLogRingCapacity;
using logging::kRecordWireSize;
using logging::LogEventType;
using logging::LogRecord;
using logging::LogRingBuffer;

namespace {

LogRecord makeRecord(uint32_t epoch, LogEventType type, uint8_t arg, int16_t centi,
                     uint16_t detail) {
    LogRecord r;
    r.epoch = epoch;
    r.minuteOfDay = static_cast<uint16_t>(epoch % 1440u);
    r.type = static_cast<uint8_t>(type);
    r.arg = arg;
    r.centiCelsius = centi;
    r.detail = detail;
    return r;
}

void test_encode_decode_roundtrip() {
    const LogRecord in = makeRecord(1700000000u, LogEventType::kTemperature, 3, 2543, 0);
    uint8_t wire[kRecordWireSize];
    TEST_ASSERT_EQUAL_UINT(kRecordWireSize, encodeRecord(in, wire, sizeof(wire)));

    LogRecord out;
    TEST_ASSERT_TRUE(decodeRecord(wire, sizeof(wire), out));
    TEST_ASSERT_EQUAL_UINT32(in.epoch, out.epoch);
    TEST_ASSERT_EQUAL_UINT16(in.minuteOfDay, out.minuteOfDay);
    TEST_ASSERT_EQUAL_UINT8(in.type, out.type);
    TEST_ASSERT_EQUAL_UINT8(in.arg, out.arg);
    TEST_ASSERT_EQUAL_INT16(in.centiCelsius, out.centiCelsius);
    TEST_ASSERT_EQUAL_UINT16(in.detail, out.detail);
}

void test_encode_decode_negative_temperature() {
    const LogRecord in = makeRecord(0, LogEventType::kTemperature, 0, -1275, 0);
    uint8_t wire[kRecordWireSize];
    encodeRecord(in, wire, sizeof(wire));
    LogRecord out;
    TEST_ASSERT_TRUE(decodeRecord(wire, sizeof(wire), out));
    TEST_ASSERT_EQUAL_INT16(-1275, out.centiCelsius);
}

void test_decode_rejects_bad_crc() {
    const LogRecord in = makeRecord(42, LogEventType::kChannelOn, 1, 0, 0);
    uint8_t wire[kRecordWireSize];
    encodeRecord(in, wire, sizeof(wire));
    wire[2] ^= 0xFF;  // corrupt a payload byte
    LogRecord out;
    TEST_ASSERT_FALSE(decodeRecord(wire, sizeof(wire), out));
}

void test_decode_rejects_short_input() {
    const LogRecord in = makeRecord(42, LogEventType::kAlertSet, 2, 0, 0x0004);
    uint8_t wire[kRecordWireSize];
    encodeRecord(in, wire, sizeof(wire));
    LogRecord out;
    TEST_ASSERT_FALSE(decodeRecord(wire, kRecordWireSize - 1, out));
}

void test_encode_rejects_small_buffer() {
    const LogRecord in = makeRecord(1, LogEventType::kBoot, 0, 0, 0);
    uint8_t tiny[kRecordWireSize - 1];
    TEST_ASSERT_EQUAL_UINT(0, encodeRecord(in, tiny, sizeof(tiny)));
}

void test_ring_push_and_order() {
    LogRingBuffer ring;
    TEST_ASSERT_TRUE(ring.empty());
    for (uint8_t i = 0; i < 5; ++i) {
        ring.push(makeRecord(i, LogEventType::kChannelOn, i, 0, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(5, ring.size());
    // Oldest-first ordering.
    for (uint8_t i = 0; i < 5; ++i) {
        TEST_ASSERT_EQUAL_UINT32(i, ring.at(i).epoch);
        TEST_ASSERT_EQUAL_UINT8(i, ring.at(i).arg);
    }
}

void test_ring_overwrites_oldest_on_wrap() {
    LogRingBuffer ring;
    const uint16_t total = kLogRingCapacity + 10u;
    for (uint16_t i = 0; i < total; ++i) {
        ring.push(makeRecord(i, LogEventType::kTemperature, 0, 0, 0));
    }
    TEST_ASSERT_EQUAL_UINT8(kLogRingCapacity, ring.size());
    // Oldest retained record is the (total - capacity)-th pushed.
    const uint32_t expectedOldest = total - kLogRingCapacity;
    TEST_ASSERT_EQUAL_UINT32(expectedOldest, ring.at(0).epoch);
    TEST_ASSERT_EQUAL_UINT32(total - 1, ring.at(static_cast<uint8_t>(ring.size() - 1)).epoch);
}

void test_ring_clear() {
    LogRingBuffer ring;
    ring.push(makeRecord(1, LogEventType::kBoot, 0, 0, 0));
    ring.clear();
    TEST_ASSERT_TRUE(ring.empty());
    TEST_ASSERT_EQUAL_UINT8(0, ring.size());
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_encode_decode_roundtrip);
    RUN_TEST(test_encode_decode_negative_temperature);
    RUN_TEST(test_decode_rejects_bad_crc);
    RUN_TEST(test_decode_rejects_short_input);
    RUN_TEST(test_encode_rejects_small_buffer);
    RUN_TEST(test_ring_push_and_order);
    RUN_TEST(test_ring_overwrites_oldest_on_wrap);
    RUN_TEST(test_ring_clear);
    return UNITY_END();
}
