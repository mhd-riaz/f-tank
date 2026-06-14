#include "network/NetworkStore.h"

#include <Preferences.h>
#include <cstring>

namespace network {
namespace {

constexpr char kNamespace[] = "ftnet";
constexpr char kKeySsid[] = "ssid";
constexpr char kKeyPass[] = "pass";
constexpr char kKeyToken[] = "token";

}  // namespace

void NetworkStore::begin() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
        return;  // namespace absent yet -> all empty (first boot)
    }
    prefs.getString(kKeySsid, ssid_, sizeof(ssid_));
    prefs.getString(kKeyPass, password_, sizeof(password_));
    prefs.getString(kKeyToken, token_, sizeof(token_));
    prefs.end();
}

bool NetworkStore::setWifiCredentials(const char* ssid, const char* password) {
    if (ssid == nullptr || password == nullptr) {
        return false;
    }
    if (strlen(ssid) == 0 || strlen(ssid) >= sizeof(ssid_) ||
        strlen(password) >= sizeof(password_)) {
        return false;  // bounds guard (NFR-8)
    }

    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    // SSID is non-empty (guarded above); password may be empty (open network),
    // so only the SSID write is required to report bytes written.
    const bool ssidOk = prefs.putString(kKeySsid, ssid) > 0;
    prefs.putString(kKeyPass, password);
    prefs.end();
    if (ssidOk) {
        snprintf(ssid_, sizeof(ssid_), "%s", ssid);
        snprintf(password_, sizeof(password_), "%s", password);
    }
    return ssidOk;
}

bool NetworkStore::setToken(const char* token) {
    if (token == nullptr || strlen(token) == 0 || strlen(token) >= sizeof(token_)) {
        return false;
    }
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    const bool ok = prefs.putString(kKeyToken, token) > 0;
    prefs.end();
    if (ok) {
        snprintf(token_, sizeof(token_), "%s", token);
    }
    return ok;
}

void NetworkStore::clear() {
    Preferences prefs;
    if (prefs.begin(kNamespace, /*readOnly=*/false)) {
        prefs.clear();
        prefs.end();
    }
    ssid_[0] = '\0';
    password_[0] = '\0';
    token_[0] = '\0';
}

}  // namespace network
