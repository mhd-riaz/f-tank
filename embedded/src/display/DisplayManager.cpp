#include "display/DisplayManager.h"

#include "display/ScreenFormat.h"

namespace display {
namespace {

const char* timeSourceLabel(ftime::TimeSource src) {
    switch (src) {
        case ftime::TimeSource::kNtp:
            return "NTP";
        case ftime::TimeSource::kRtc:
            return "RTC";
        case ftime::TimeSource::kSoftClock:
        default:
            return "SOFT";
    }
}

}  // namespace

void DisplayManager::begin() {
    u8g2_.begin();
    initialized_ = true;
}

void DisplayManager::update(uint32_t nowMs) {
    if (!initialized_) {
        return;
    }
    if (nowMs - lastRotateMs_ < kScreenDurationMs && lastRotateMs_ != 0) {
        return;
    }
    lastRotateMs_ = nowMs;

    switch (screen_) {
        case 0:
            renderTimeDate();
            break;
        case 1:
            renderChannels();
            break;
        case 2:
            renderTemperature();
            break;
        default:
            renderStatus();
            break;
    }
    screen_ = static_cast<uint8_t>((screen_ + 1) % kScreenCount);
}

void DisplayManager::renderTimeDate() {
    char line1[16];
    char line2[16];
    const ftime::WallClock wc = time_.now();
    formatTime(line1, sizeof(line1), wc);
    formatDate(line2, sizeof(line2), wc);

    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_logisoso16_tf);
    u8g2_.setCursor(0, 16);
    u8g2_.print(line1);
    u8g2_.setFont(u8g2_font_6x10_tr);
    u8g2_.setCursor(0, 30);
    u8g2_.print(line2);
    u8g2_.sendBuffer();
}

void DisplayManager::renderChannels() {
    bool states[channel::kMaxChannels] = {false};
    const uint8_t count = channels_.count();
    for (uint8_t i = 0; i < count; ++i) {
        states[i] = channels_.isEnergized(i);
    }
    char line[4 + channel::kMaxChannels];
    formatChannelStates(line, sizeof(line), states, count);

    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_6x12_tf);
    u8g2_.setCursor(0, 12);
    u8g2_.print("Channels");
    u8g2_.setCursor(0, 28);
    u8g2_.print(line);
    u8g2_.sendBuffer();
}

void DisplayManager::renderTemperature() {
    char line[24];
    formatTemperature(line, sizeof(line), temp_.celsius(), temp_.hasValidReading());

    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_logisoso16_tf);
    u8g2_.setCursor(0, 24);
    u8g2_.print(line);
    u8g2_.sendBuffer();
}

void DisplayManager::renderStatus() {
    char line2[20];
    snprintf(line2, sizeof(line2), "Clk:%s Alm:%s", timeSourceLabel(time_.source()),
             alerts_.anyActive() ? "YES" : "no");

    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_6x12_tf);
    u8g2_.setCursor(0, 12);
    u8g2_.print("Status");
    u8g2_.setCursor(0, 28);
    u8g2_.print(line2);
    u8g2_.sendBuffer();
}

}  // namespace display
