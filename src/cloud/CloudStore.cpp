#include "cloud/CloudStore.h"

#include <Preferences.h>
#include <cstring>

#include "cloud/CloudTopics.h"

namespace cloud {
namespace {

constexpr char kNamespace[] = "ftcloud";
constexpr char kKeyHost[] = "host";
constexpr char kKeyPort[] = "port";
constexpr char kKeyDevId[] = "devid";
constexpr char kKeySub[] = "sub";
constexpr char kKeyCa[] = "ca";
constexpr char kKeyCert[] = "cert";
constexpr char kKeyKey[] = "key";

}  // namespace

void CloudStore::begin() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
        return;  // first boot: namespace absent -> all empty
    }
    prefs.getString(kKeyHost, endpoint_, sizeof(endpoint_));
    port_ = prefs.getUShort(kKeyPort, 8883);
    prefs.getString(kKeyDevId, deviceId_, sizeof(deviceId_));
    subscriptionActive_ = prefs.getBool(kKeySub, false);
    prefs.getString(kKeyCa, caCert_, sizeof(caCert_));
    prefs.getString(kKeyCert, clientCert_, sizeof(clientCert_));
    prefs.getString(kKeyKey, clientKey_, sizeof(clientKey_));
    prefs.end();
}

bool CloudStore::isProvisioned() const {
    return endpoint_[0] != '\0' && isValidDeviceId(deviceId_) && hasTlsMaterial();
}

void CloudStore::setSubscriptionActive(bool active) {
    subscriptionActive_ = active;
    Preferences prefs;
    if (prefs.begin(kNamespace, /*readOnly=*/false)) {
        prefs.putBool(kKeySub, active);
        prefs.end();
    }
}

bool CloudStore::setEndpoint(const char* host, uint16_t port, const char* deviceId) {
    if (host == nullptr || strlen(host) == 0 || strlen(host) >= sizeof(endpoint_)) {
        return false;
    }
    if (!isValidDeviceId(deviceId)) {
        return false;  // reject ids that could break topic construction (NFR-8)
    }
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    const bool ok = prefs.putString(kKeyHost, host) > 0 && prefs.putUShort(kKeyPort, port) > 0 &&
                    prefs.putString(kKeyDevId, deviceId) > 0;
    prefs.end();
    if (ok) {
        snprintf(endpoint_, sizeof(endpoint_), "%s", host);
        port_ = port;
        snprintf(deviceId_, sizeof(deviceId_), "%s", deviceId);
    }
    return ok;
}

bool CloudStore::setTlsMaterial(const char* caCert, const char* clientCert, const char* clientKey) {
    if (caCert == nullptr || clientCert == nullptr || clientKey == nullptr) {
        return false;
    }
    if (strlen(caCert) >= sizeof(caCert_) || strlen(clientCert) >= sizeof(clientCert_) ||
        strlen(clientKey) >= sizeof(clientKey_)) {
        return false;
    }
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    const bool ok = prefs.putString(kKeyCa, caCert) > 0 &&
                    prefs.putString(kKeyCert, clientCert) > 0 &&
                    prefs.putString(kKeyKey, clientKey) > 0;
    prefs.end();
    if (ok) {
        snprintf(caCert_, sizeof(caCert_), "%s", caCert);
        snprintf(clientCert_, sizeof(clientCert_), "%s", clientCert);
        snprintf(clientKey_, sizeof(clientKey_), "%s", clientKey);
    }
    return ok;
}

void CloudStore::clear() {
    Preferences prefs;
    if (prefs.begin(kNamespace, /*readOnly=*/false)) {
        prefs.clear();
        prefs.end();
    }
    endpoint_[0] = '\0';
    port_ = 8883;
    deviceId_[0] = '\0';
    subscriptionActive_ = false;
    caCert_[0] = '\0';
    clientCert_[0] = '\0';
    clientKey_[0] = '\0';
}

}  // namespace cloud
