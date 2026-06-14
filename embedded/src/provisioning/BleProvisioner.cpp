#include "provisioning/BleProvisioner.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "api/TokenGenerator.h"

namespace provisioning {
namespace {

// 128-bit UUIDs for the f-tank provisioning service + characteristics.
constexpr char kSvcUuid[] = "6e6b0001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kSsidUuid[] = "6e6b0002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kPassUuid[] = "6e6b0003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kTzUuid[] = "6e6b0004-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kApplyUuid[] = "6e6b0005-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kStatusUuid[] = "6e6b0006-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kTokenUuid[] = "6e6b0007-b5a3-f393-e0a9-e50e24dcca9e";

constexpr uint32_t kTeardownDelayMs = 1500;  // let the app read the token first

BleProvisioner* g_self = nullptr;
NimBLECharacteristic* g_statusChar = nullptr;
NimBLECharacteristic* g_tokenChar = nullptr;

void notifyStatus(ProvStatus s) {
    if (g_statusChar != nullptr) {
        uint8_t v = static_cast<uint8_t>(s);
        g_statusChar->setValue(&v, 1);
        g_statusChar->notify();
    }
}

class WriteCallbacks : public NimBLECharacteristicCallbacks {
  public:
    explicit WriteCallbacks(char kind) : kind_(kind) {}
    void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) override {
        if (g_self == nullptr) {
            return;
        }
        const std::string val = chr->getValue();
        switch (kind_) {
            case 's':
                g_self->onSsidWrite(val.c_str());
                break;
            case 'p':
                g_self->onPasswordWrite(val.c_str());
                break;
            case 't':
                g_self->onTzWrite(val.c_str());
                break;
            case 'a':
                g_self->onApplyCommand();
                break;
            default:
                break;
        }
    }

  private:
    char kind_;
};

class ServerCallbacks : public NimBLEServerCallbacks {
  public:
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
        if (g_self != nullptr) {
            g_self->onClientConnected();
        }
    }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
        if (g_self != nullptr) {
            g_self->onClientDisconnected();
        }
    }
};

}  // namespace

void BleProvisioner::beginIfUnprovisioned() {
    if (netStore_.hasWifiCredentials()) {
        return;  // already provisioned -> keep BLE off (saves RAM, smaller surface)
    }
    start();
}

void BleProvisioner::start() {
    if (active_) {
        return;
    }
    g_self = this;
    session_.reset();
    setupGatt();
    active_ = true;
    status_ = ProvStatus::kAdvertising;
}

void BleProvisioner::setupGatt() {
    NimBLEDevice::init("f-tank-setup");
    NimBLEDevice::setPower(ESP_PWR_LVL_P3);
    // Require encryption + bonding so credentials are never sent in the clear.
    NimBLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/false, /*sc=*/true);

    NimBLEServer* server = NimBLEDevice::createServer();
    static ServerCallbacks serverCb;
    server->setCallbacks(&serverCb);

    NimBLEService* svc = server->createService(kSvcUuid);

    const uint32_t writeEnc = NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC;
    const uint32_t readNotifyEnc =
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::NOTIFY;

    static WriteCallbacks ssidCb('s'), passCb('p'), tzCb('t'), applyCb('a');
    svc->createCharacteristic(kSsidUuid, writeEnc)->setCallbacks(&ssidCb);
    svc->createCharacteristic(kPassUuid, writeEnc)->setCallbacks(&passCb);
    svc->createCharacteristic(kTzUuid, writeEnc)->setCallbacks(&tzCb);
    svc->createCharacteristic(kApplyUuid, writeEnc)->setCallbacks(&applyCb);

    g_statusChar = svc->createCharacteristic(kStatusUuid, readNotifyEnc);
    uint8_t initStatus = static_cast<uint8_t>(ProvStatus::kAdvertising);
    g_statusChar->setValue(&initStatus, 1);
    g_tokenChar = svc->createCharacteristic(kTokenUuid, readNotifyEnc);

    // NimBLE 2.x starts services automatically when the server starts.
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(kSvcUuid);
    adv->enableScanResponse(true);
    NimBLEDevice::startAdvertising();
}

void BleProvisioner::onClientConnected() {
    status_ = ProvStatus::kConnected;
    notifyStatus(status_);
}

void BleProvisioner::onClientDisconnected() {
    if (active_ && status_ != ProvStatus::kApplied) {
        status_ = ProvStatus::kAdvertising;
        NimBLEDevice::startAdvertising();
    }
}

void BleProvisioner::onSsidWrite(const char* value) {
    session_.setSsid(value);
}
void BleProvisioner::onPasswordWrite(const char* value) {
    session_.setPassword(value);
}
void BleProvisioner::onTzWrite(const char* value) {
    session_.setTz(value);
}

void BleProvisioner::onApplyCommand() {
    applyRequested_ = true;  // committed in update() on the main context
}

void BleProvisioner::commit() {
    if (!session_.isReady()) {
        status_ = ProvStatus::kError;
        notifyStatus(status_);
        return;
    }

    // Persist WiFi credentials (NFR-5) and timezone.
    if (!netStore_.setWifiCredentials(session_.ssid(), session_.password())) {
        status_ = ProvStatus::kError;
        notifyStatus(status_);
        return;
    }
    snprintf(configStore_.config().tz, sizeof(configStore_.config().tz), "%s", session_.tz());
    configStore_.save();

    // Issue (or reuse) the device bearer token and hand it to the app.
    if (!netStore_.hasToken()) {
        char token[network::kTokenSize] = {0};
        if (api::generateToken(token, sizeof(token))) {
            netStore_.setToken(token);
        }
    }
    if (g_tokenChar != nullptr) {
        g_tokenChar->setValue(reinterpret_cast<const uint8_t*>(netStore_.token()),
                              strlen(netStore_.token()));
        g_tokenChar->notify();
    }

    status_ = ProvStatus::kApplied;
    notifyStatus(status_);
    justProvisioned_ = true;
    teardownPending_ = true;
    teardownAtMs_ = millis() + kTeardownDelayMs;
}

void BleProvisioner::update(uint32_t nowMs) {
    if (!active_) {
        return;
    }
    if (applyRequested_) {
        applyRequested_ = false;
        commit();
    }
    if (teardownPending_ && static_cast<int32_t>(nowMs - teardownAtMs_) >= 0) {
        teardown();
    }
}

void BleProvisioner::teardown() {
    teardownPending_ = false;
    active_ = false;
    g_statusChar = nullptr;
    g_tokenChar = nullptr;
    NimBLEDevice::deinit(/*clearAll=*/true);  // reclaim BLE RAM (AD-6)
    status_ = ProvStatus::kIdle;
}

}  // namespace provisioning
