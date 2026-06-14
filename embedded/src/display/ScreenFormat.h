#ifndef DISPLAY_SCREEN_FORMAT_H
#define DISPLAY_SCREEN_FORMAT_H

#include <cstdint>
#include <cstdio>

#include "sensor/Ds18b20Math.h"
#include "time/WallClock.h"

/**
 * @file ScreenFormat.h
 * @brief Pure text builders for OLED lines (bounded, ASCII-only, NFR-9).
 *
 * Hardware-free so formatting is unit-tested natively. All writers use bounded
 * snprintf into caller-provided buffers and never use a runtime string as a
 * format argument. ASCII only (no multibyte degree symbol).
 */
namespace display {

/// Format "HH:MM:SS" into buf. Returns chars written (excl. null).
inline int formatTime(char* buf, size_t size, const ftime::WallClock& wc) {
    return snprintf(buf, size, "%02u:%02u:%02u", static_cast<unsigned>(wc.hour),
                    static_cast<unsigned>(wc.minute), static_cast<unsigned>(wc.second));
}

/// Format "DD/MM/YYYY" into buf.
inline int formatDate(char* buf, size_t size, const ftime::WallClock& wc) {
    return snprintf(buf, size, "%02u/%02u/%04u", static_cast<unsigned>(wc.day),
                    static_cast<unsigned>(wc.month), static_cast<unsigned>(wc.year));
}

/// Format temperature as "Temp: 25.1 C" or "Temp: --.- C" when invalid.
inline int formatTemperature(char* buf, size_t size, float celsius, bool valid) {
    if (!valid) {
        return snprintf(buf, size, "Temp: --.- C");
    }
    // Manual one-decimal split avoids %f bloat and keeps it ASCII.
    const int tenths = static_cast<int>(celsius * 10.0f + (celsius >= 0 ? 0.5f : -0.5f));
    return snprintf(buf, size, "Temp: %d.%d C", tenths / 10, (tenths < 0 ? -tenths : tenths) % 10);
}

/// Format compact channel states like "Ch:1010" (1=on) into buf.
inline int formatChannelStates(char* buf, size_t size, const bool* energized, uint8_t count) {
    if (size == 0) {
        return 0;
    }
    int written = snprintf(buf, size, "Ch:");
    for (uint8_t i = 0; i < count && static_cast<size_t>(written) + 1 < size; ++i) {
        buf[written++] = energized[i] ? '1' : '0';
    }
    buf[written] = '\0';
    return written;
}

}  // namespace display

#endif  // DISPLAY_SCREEN_FORMAT_H
