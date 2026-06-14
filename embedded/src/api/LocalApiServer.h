#ifndef API_LOCAL_API_SERVER_H
#define API_LOCAL_API_SERVER_H

#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "api/ApiValidation.h"
#include "api/ConfigBroker.h"
#include "api/StatusBroker.h"
#include "log/EventLogger.h"
#include "network/NetworkStore.h"
#include "ota/OtaUpdater.h"
#include "storage/PersistentConfig.h"

/**
 * @file LocalApiServer.h
 * @brief Versioned local REST + WebSocket API (AD-3/AD-4, FR-33/35).
 *
 * Runs on the networking context. Every /api/v1 route requires a valid bearer
 * token (constant-time check). Config writes are validated (NFR-8) then handed
 * to the control loop via ConfigBroker; live state is read via StatusBroker.
 * Plain HTTP on the LAN by design (AD-4) — TLS is provided by the cloud path.
 */
namespace api {

class LocalApiServer {
  public:
    LocalApiServer(network::NetworkStore& netStore, ConfigBroker& configBroker,
                   StatusBroker& statusBroker, ota::OtaUpdater& otaUpdater,
                   logging::EventLogger& eventLogger)
        : server_(80),
          ws_("/api/v1/ws"),
          netStore_(netStore),
          configBroker_(configBroker),
          statusBroker_(statusBroker),
          ota_(otaUpdater),
          logger_(eventLogger) {}

    /// Register routes and start listening. Call once WiFi is connected.
    void begin();

    /// Publish the current applied config so GET /config can serve it (control loop).
    void publishConfig(const storage::PersistentConfig& cfg);

    /// Periodic servicing: prune WS clients, push status. Networking context.
    void loop(uint32_t nowMs);

    /// True once a verified OTA image is staged and a reboot should occur.
    bool otaRebootPending() const {
        return ota_.rebootPending();
    }

    /// Scope of a pending factory/wifi reset request (FR-10), or kNone.
    ResetScope pendingReset() const {
        return resetScope_;
    }

    /// True when a reset has been requested AND its grace period (for the HTTP
    /// response to flush) has elapsed, so the control loop may wipe + reboot.
    bool resetDue(uint32_t nowMs) const;

  private:
    bool authorize(AsyncWebServerRequest* request) const;
    void handleGetInfo(AsyncWebServerRequest* request);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handlePutConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleGetLogs(AsyncWebServerRequest* request);
    void handleSetRecording(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleReset(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleOtaUpload(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index,
                         size_t total);
    void registerRoutes();
    void pushStatusToClients();

    AsyncWebServer server_;
    AsyncWebSocket ws_;
    network::NetworkStore& netStore_;
    ConfigBroker& configBroker_;
    StatusBroker& statusBroker_;
    ota::OtaUpdater& ota_;
    logging::EventLogger& logger_;

    SemaphoreHandle_t configMutex_ = nullptr;
    storage::PersistentConfig configView_;
    bool started_ = false;
    uint32_t lastPushMs_ = 0;
    ResetScope resetScope_ = ResetScope::kNone;
    uint32_t resetAtMs_ = 0;
};

}  // namespace api

#endif  // API_LOCAL_API_SERVER_H
