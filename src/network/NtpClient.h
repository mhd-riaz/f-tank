#ifndef NETWORK_NTP_CLIENT_H
#define NETWORK_NTP_CLIENT_H

#include <cstdint>

#include "time/TimeService.h"

/**
 * @file NtpClient.h
 * @brief SNTP time sync that feeds local time into TimeService (FR-5/6/7a).
 *
 * Uses the ESP-IDF SNTP path (configTzTime) so the POSIX TZ string applies DST
 * automatically. Non-blocking: polls for a valid clock and pushes it once
 * obtained. Raises an NTP-failure flag if sync does not complete in time.
 */
namespace network {

class NtpClient {
  public:
    explicit NtpClient(ftime::TimeService& timeService) : timeService_(timeService) {}

    /// Configure SNTP + timezone and start a sync attempt. Call once WiFi is up.
    void begin(const char* posixTz);

    /// Cooperative servicing; non-blocking. Call frequently while connected.
    void update(uint32_t nowMs);

    /// Notify that WiFi just (re)connected — (re)arms a sync attempt.
    void onWifiConnected(uint32_t nowMs);

    bool hasSynced() const {
        return synced_;
    }

    /// True if a sync was expected but has not completed within the timeout.
    bool inFailure() const {
        return failed_;
    }

  private:
    bool tryReadSyncedTime();

    ftime::TimeService& timeService_;
    char tz_[48] = "UTC0";
    bool sntpStarted_ = false;
    bool synced_ = false;
    bool failed_ = false;
    uint32_t attemptStartMs_ = 0;
    uint32_t lastResyncMs_ = 0;
};

}  // namespace network

#endif  // NETWORK_NTP_CLIENT_H
