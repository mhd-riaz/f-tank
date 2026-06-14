#include "api/LocalApiServer.h"

#include <ArduinoJson.h>

#include "api/ApiValidation.h"
#include "api/AuthToken.h"
#include "network/NetworkStore.h"
#include "ota/FirmwareVersion.h"
#include "ota/OtaTypes.h"

namespace api {
namespace {

constexpr char kFirmwareVersion[] = "2.0.0-dev";

void serializeStatus(const StatusSnapshot& s, JsonObject obj) {
    obj["tempC"] = s.temperatureValid ? s.temperatureC : (float)-127.0;
    obj["tempValid"] = s.temperatureValid;
    obj["alerts"] = s.activeAlertMask;
    obj["timeSource"] = s.timeSource;
    obj["wifiState"] = s.wifiState;
    obj["minuteOfDay"] = s.minuteOfDay;
    JsonArray ch = obj["channels"].to<JsonArray>();
    for (uint8_t i = 0; i < s.channelCount; ++i) {
        ch.add(s.channelEnergized[i]);
    }
}

void serializeConfig(const storage::PersistentConfig& cfg, JsonObject obj) {
    obj["tz"] = cfg.tz;
    JsonObject th = obj["thresholds"].to<JsonObject>();
    th["lowC"] = cfg.thresholds.lowC;
    th["highC"] = cfg.thresholds.highC;
    th["hysteresisC"] = cfg.thresholds.hysteresisC;

    JsonArray chans = obj["channels"].to<JsonArray>();
    for (uint8_t i = 0; i < cfg.channelCount; ++i) {
        const storage::PersistentChannel& pc = cfg.channels[i];
        JsonObject c = chans.add<JsonObject>();
        c["name"] = pc.config.name;
        c["enabled"] = pc.config.enabled;
        c["cutOnOverTemp"] = pc.config.cutOnOverTemp;              // read-only (provisioned)
        c["polarity"] = static_cast<uint8_t>(pc.config.polarity);  // read-only
        JsonObject sch = c["schedule"].to<JsonObject>();
        sch["inverted"] = pc.schedule.inverted;
        JsonArray wins = sch["windows"].to<JsonArray>();
        for (uint8_t w = 0; w < pc.schedule.windowCount; ++w) {
            JsonObject win = wins.add<JsonObject>();
            win["start"] = pc.schedule.windows[w].startMin;
            win["end"] = pc.schedule.windows[w].endMin;
        }
    }
}

void sendJsonError(AsyncWebServerRequest* request, int code, const char* msg) {
    JsonDocument doc;
    doc["error"] = msg;
    AsyncResponseStream* res = request->beginResponseStream("application/json");
    res->setCode(code);
    serializeJson(doc, *res);
    request->send(res);
}

}  // namespace

void LocalApiServer::begin() {
    if (started_) {
        return;
    }
    if (configMutex_ == nullptr) {
        configMutex_ = xSemaphoreCreateMutex();
    }
    registerRoutes();
    server_.begin();
    started_ = true;
}

void LocalApiServer::publishConfig(const storage::PersistentConfig& cfg) {
    if (configMutex_ == nullptr) {
        configMutex_ = xSemaphoreCreateMutex();
    }
    if (xSemaphoreTake(configMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        configView_ = cfg;
        xSemaphoreGive(configMutex_);
    }
}

bool LocalApiServer::authorize(AsyncWebServerRequest* request) const {
    // Lock down the API entirely until a device token has been provisioned.
    if (!netStore_.hasToken()) {
        return false;
    }
    if (!request->hasHeader("Authorization")) {
        return false;
    }
    char token[network::kTokenSize] = {0};
    const AsyncWebHeader* h = request->getHeader("Authorization");
    if (!extractBearerToken(h->value().c_str(), token, sizeof(token))) {
        return false;
    }
    return constantTimeEquals(token, netStore_.token(), network::kTokenSize);
}

void LocalApiServer::registerRoutes() {
    // Unauthenticated liveness probe (no sensitive data).
    server_.on("/healthz", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"ok\":true}");
    });

    server_.on("/api/v1/info", HTTP_GET,
               [this](AsyncWebServerRequest* request) { handleGetInfo(request); });
    server_.on("/api/v1/status", HTTP_GET,
               [this](AsyncWebServerRequest* request) { handleGetStatus(request); });
    server_.on("/api/v1/config", HTTP_GET,
               [this](AsyncWebServerRequest* request) { handleGetConfig(request); });

    // PUT /config with a body handler (accumulates into request->_tempObject not
    // needed; ESPAsyncWebServer delivers the full body via the onBody callback).
    server_.on(
        "/api/v1/config", HTTP_PUT,
        [](AsyncWebServerRequest* request) { /* response sent from body handler */ }, nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index,
               size_t total) {
            if (index == 0 && len == total) {
                handlePutConfig(request, data, len);
            } else {
                sendJsonError(request, 413, "body too large");
            }
        });

    ws_.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType, void*, uint8_t*, size_t) {
        // Read-only live push; inbound WS messages are ignored.
    });
    server_.addHandler(&ws_);

    // Signed OTA upload (FR-38/39): metadata in headers, firmware in the body.
    server_.on(
        "/api/v1/ota", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* response sent from body handler */ }, nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index,
               size_t total) { handleOtaUpload(request, data, len, index, total); });

    server_.onNotFound(
        [](AsyncWebServerRequest* request) { sendJsonError(request, 404, "not found"); });
}

void LocalApiServer::handleGetInfo(AsyncWebServerRequest* request) {
    if (!authorize(request)) {
        sendJsonError(request, 401, "unauthorized");
        return;
    }
    JsonDocument doc;
    doc["firmware"] = kFirmwareVersion;
    doc["configVersion"] = storage::kConfigVersion;
    if (xSemaphoreTake(configMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        doc["channelCount"] = configView_.channelCount;
        xSemaphoreGive(configMutex_);
    }
    AsyncResponseStream* res = request->beginResponseStream("application/json");
    serializeJson(doc, *res);
    request->send(res);
}

void LocalApiServer::handleGetStatus(AsyncWebServerRequest* request) {
    if (!authorize(request)) {
        sendJsonError(request, 401, "unauthorized");
        return;
    }
    StatusSnapshot snap;
    if (!statusBroker_.read(snap)) {
        sendJsonError(request, 503, "status unavailable");
        return;
    }
    JsonDocument doc;
    serializeStatus(snap, doc.to<JsonObject>());
    AsyncResponseStream* res = request->beginResponseStream("application/json");
    serializeJson(doc, *res);
    request->send(res);
}

void LocalApiServer::handleGetConfig(AsyncWebServerRequest* request) {
    if (!authorize(request)) {
        sendJsonError(request, 401, "unauthorized");
        return;
    }
    JsonDocument doc;
    if (xSemaphoreTake(configMutex_, pdMS_TO_TICKS(100)) != pdTRUE) {
        sendJsonError(request, 503, "config busy");
        return;
    }
    serializeConfig(configView_, doc.to<JsonObject>());
    xSemaphoreGive(configMutex_);
    AsyncResponseStream* res = request->beginResponseStream("application/json");
    serializeJson(doc, *res);
    request->send(res);
}

void LocalApiServer::handlePutConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    if (!authorize(request)) {
        sendJsonError(request, 401, "unauthorized");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, data, len) != DeserializationError::Ok) {
        sendJsonError(request, 400, "invalid json");
        return;
    }

    // Start from the current applied config; only user-editable fields are
    // overlaid (polarity/gpio/cutOnOverTemp stay as provisioned, FR-12).
    storage::PersistentConfig next;
    if (xSemaphoreTake(configMutex_, pdMS_TO_TICKS(100)) != pdTRUE) {
        sendJsonError(request, 503, "config busy");
        return;
    }
    next = configView_;
    xSemaphoreGive(configMutex_);

    if (doc["tz"].is<const char*>()) {
        const char* tz = doc["tz"];
        if (!isValidTz(tz)) {
            sendJsonError(request, 400, "invalid tz");
            return;
        }
        snprintf(next.tz, sizeof(next.tz), "%s", tz);
    }

    if (doc["thresholds"].is<JsonObject>()) {
        JsonObject th = doc["thresholds"];
        alert::TempThresholds t = next.thresholds;
        if (th["lowC"].is<float>())
            t.lowC = th["lowC"];
        if (th["highC"].is<float>())
            t.highC = th["highC"];
        if (th["hysteresisC"].is<float>())
            t.hysteresisC = th["hysteresisC"];
        if (!isValidThresholds(t)) {
            sendJsonError(request, 400, "invalid thresholds");
            return;
        }
        next.thresholds = t;
    }

    if (doc["channels"].is<JsonArray>()) {
        JsonArray chans = doc["channels"];
        if (chans.size() != next.channelCount) {
            sendJsonError(request, 400, "channel count mismatch");
            return;
        }
        uint8_t idx = 0;
        for (JsonObject c : chans) {
            storage::PersistentChannel& pc = next.channels[idx];

            if (c["name"].is<const char*>()) {
                const char* name = c["name"];
                if (!isValidChannelName(name)) {
                    sendJsonError(request, 400, "invalid channel name");
                    return;
                }
                snprintf(pc.config.name, sizeof(pc.config.name), "%s", name);
            }
            if (c["enabled"].is<bool>()) {
                pc.config.enabled = c["enabled"];
            }
            if (c["schedule"].is<JsonObject>()) {
                JsonObject sch = c["schedule"];
                schedule::ChannelSchedule ns;
                ns.inverted = sch["inverted"].is<bool>() ? (bool)sch["inverted"] : false;
                ns.windowCount = 0;
                if (sch["windows"].is<JsonArray>()) {
                    JsonArray wins = sch["windows"];
                    if (wins.size() > schedule::kMaxWindowsPerChannel) {
                        sendJsonError(request, 400, "too many windows");
                        return;
                    }
                    for (JsonObject win : wins) {
                        ns.windows[ns.windowCount].startMin =
                            static_cast<uint16_t>(win["start"] | 0);
                        ns.windows[ns.windowCount].endMin = static_cast<uint16_t>(win["end"] | 0);
                        ns.windowCount++;
                    }
                }
                if (!isValidSchedule(ns)) {
                    sendJsonError(request, 400, "invalid schedule");
                    return;
                }
                pc.schedule = ns;
            }
            ++idx;
        }
    }

    if (!configBroker_.stage(next)) {
        sendJsonError(request, 503, "apply busy");
        return;
    }
    request->send(202, "application/json", "{\"status\":\"accepted\"}");
}

void LocalApiServer::handleOtaUpload(AsyncWebServerRequest* request, uint8_t* data, size_t len,
                                     size_t index, size_t total) {
    // First chunk: authorize, parse headers, and open the OTA session.
    if (index == 0) {
        if (!authorize(request)) {
            sendJsonError(request, 401, "unauthorized");
            return;
        }
        if (!request->hasHeader("X-FW-Version") || !request->hasHeader("X-FW-SHA256") ||
            !request->hasHeader("X-FW-Signature")) {
            sendJsonError(request, 400, "missing ota headers");
            return;
        }

        ota::FirmwareVersion imageVer;
        if (!ota::parseVersion(request->getHeader("X-FW-Version")->value().c_str(), imageVer)) {
            sendJsonError(request, 400, "bad version");
            return;
        }
        ota::FirmwareVersion currentVer;
        ota::parseVersion(kFirmwareVersion, currentVer);

        uint8_t expectedSha[ota::kSha256Len] = {0};
        if (!ota::hexDecode(request->getHeader("X-FW-SHA256")->value().c_str(), expectedSha,
                            sizeof(expectedSha))) {
            sendJsonError(request, 400, "bad sha256");
            return;
        }

        const char* sigHex = request->getHeader("X-FW-Signature")->value().c_str();
        const size_t sigBytes = strlen(sigHex) / 2;
        uint8_t signature[ota::kMaxSignatureLen] = {0};
        if (sigBytes == 0 || sigBytes > sizeof(signature) ||
            !ota::hexDecode(sigHex, signature, sigBytes)) {
            sendJsonError(request, 400, "bad signature");
            return;
        }

        const ota::OtaError err = ota_.begin(static_cast<uint32_t>(total), imageVer, currentVer,
                                             expectedSha, signature, sigBytes);
        if (err != ota::OtaError::kOk) {
            sendJsonError(request, 400, "ota rejected");
            return;
        }
    }

    // Stream the chunk to flash.
    if (len > 0 && ota_.write(data, len) != ota::OtaError::kOk) {
        sendJsonError(request, 400, "ota write failed");
        return;
    }

    // Last chunk: verify signature + hash and activate the new image.
    if (index + len == total) {
        if (ota_.finish() != ota::OtaError::kOk) {
            sendJsonError(request, 400, "ota verification failed");
            return;
        }
        // Reboot is triggered by the control loop once it sees rebootPending().
        request->send(200, "application/json", "{\"status\":\"verified\",\"reboot\":true}");
    }
}

void LocalApiServer::pushStatusToClients() {
    if (ws_.count() == 0) {
        return;
    }
    StatusSnapshot snap;
    if (!statusBroker_.read(snap)) {
        return;
    }
    JsonDocument doc;
    serializeStatus(snap, doc.to<JsonObject>());
    char buf[512];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    if (n > 0 && n < sizeof(buf)) {
        ws_.textAll(buf, n);
    }
}

void LocalApiServer::loop(uint32_t nowMs) {
    if (!started_) {
        return;
    }
    ws_.cleanupClients();
    constexpr uint32_t kPushIntervalMs = 1000;
    if (nowMs - lastPushMs_ >= kPushIntervalMs) {
        lastPushMs_ = nowMs;
        pushStatusToClients();
    }
}

}  // namespace api
