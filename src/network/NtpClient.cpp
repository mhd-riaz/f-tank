#include "network/NtpClient.h"

#include <Arduino.h>
#include <time.h>

namespace network {
namespace {

constexpr char kNtp1[] = "pool.ntp.org";
constexpr char kNtp2[] = "time.google.com";
constexpr char kNtp3[] = "time.cloudflare.com";

constexpr uint32_t kSyncTimeoutMs = 30000;       // flag failure after 30 s
constexpr uint32_t kResyncIntervalMs = 3600000;  // opportunistic resync hourly
constexpr int kMinValidYear = 2020;              // sanity floor for "synced"

}  // namespace

void NtpClient::begin(const char* posixTz) {
    snprintf(tz_, sizeof(tz_), "%s", posixTz != nullptr ? posixTz : "UTC0");
    // Starts the SNTP service and applies the timezone (DST-aware).
    configTzTime(tz_, kNtp1, kNtp2, kNtp3);
    sntpStarted_ = true;
    synced_ = false;
    failed_ = false;
    attemptStartMs_ = millis();
}

void NtpClient::onWifiConnected(uint32_t nowMs) {
    if (!sntpStarted_) {
        begin(tz_);
        return;
    }
    // Re-arm a sync attempt window after a reconnect.
    failed_ = false;
    attemptStartMs_ = nowMs;
}

bool NtpClient::tryReadSyncedTime() {
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    if (local.tm_year + 1900 < kMinValidYear) {
        return false;  // clock not set yet
    }

    ftime::WallClock wc;
    wc.year = static_cast<uint16_t>(local.tm_year + 1900);
    wc.month = static_cast<uint8_t>(local.tm_mon + 1);
    wc.day = static_cast<uint8_t>(local.tm_mday);
    wc.hour = static_cast<uint8_t>(local.tm_hour);
    wc.minute = static_cast<uint8_t>(local.tm_min);
    wc.second = static_cast<uint8_t>(local.tm_sec);
    timeService_.setFromExternal(wc);  // updates RTC + marks source NTP (FR-6)
    return true;
}

void NtpClient::update(uint32_t nowMs) {
    if (!sntpStarted_) {
        return;
    }

    if (!synced_) {
        if (tryReadSyncedTime()) {
            synced_ = true;
            failed_ = false;
            lastResyncMs_ = nowMs;
        } else if (nowMs - attemptStartMs_ >= kSyncTimeoutMs) {
            failed_ = true;  // surfaced as an alert; RTC/soft clock keeps running
        }
        return;
    }

    // Opportunistic periodic resync keeps the RTC disciplined.
    if (nowMs - lastResyncMs_ >= kResyncIntervalMs) {
        if (tryReadSyncedTime()) {
            lastResyncMs_ = nowMs;
        }
    }
}

}  // namespace network
