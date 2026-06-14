# f-tank — Requirements Specification (v2)

Automated aquarium controller. This document defines requirements for the **next-generation
commercial product**, replacing the legacy Arduino-style sketch (removed; formerly `esp32/main.ino`).

- **Status:** Draft v2
- **Target hardware:** ESP32 (WiFi + BLE capable)
- **Date:** 2026-06-13

---

## 1. Product Overview

f-tank is a WiFi-connected aquarium automation controller. It drives multiple relay-controlled
appliances (lighting, CO₂, heater, circulation pump, etc.) on time-based schedules, monitors water
temperature, and is configured/monitored through a companion **Web UI app** over the local network,
with optional cloud connectivity for remote access and logging.

The product ships in tiers:

| Tier | Capabilities |
|------|--------------|
| **Basic** | Local control, scheduling, on-demand logs, extreme-temp alerts |
| **Premium** | + Local SD-card log storage |
| **Subscription** | + Cloud log storage, remote access, historical analytics |

Software tier (features) is **independent** of hardware variant (channel count). The same firmware
runs on all hardware variants:

| Variant | Relay channels | Analogy |
|---------|----------------|---------|
| **S** | 4 | base model |
| **Plus** | 8 | mid model |
| **Pro** | 16 (supports 12/16) | top model |

The firmware MUST support up to **16 channels** (`kMaxChannels = 16`) and discover the actual
channel count from device provisioning/hardware config at runtime — no per-variant build.

### Flash SKUs (cost optimization)

The firmware builds in two flash profiles via compile-time feature flags
([`embedded/src/config/features.h`](../embedded/src/config/features.h)), selected by PlatformIO environment:

| SKU | Flash | Env | Cloud | Persistent logging | OTA |
|-----|-------|-----|:-----:|:------------------:|:---:|
| **Basic** | 4 MB | `esp32_4mb_basic` | ❌ off | ❌ (on-demand stream only) | ✅ |
| **Full** | 8 MB | `esp32_8mb_full` | ✅ | ✅ (SD / filesystem) | ✅ |

Disabled features are **compiled out** (not linked), shrinking the 4 MB image so dual-OTA still
fits. **OTA is enabled on both SKUs** — the fleet must remain security-patchable. On-demand log
streaming over the local API is available on both; only *persistent* logging requires the 8 MB SKU.

---

## 2. Functional Requirements

### 2.1 Connectivity (Hybrid model)
- **FR-1** Device MUST operate **standalone** (schedules run from RTC) with no network present.
- **FR-2** Device MUST provide **local control**: the app talks directly to the ESP32 over LAN
  (HTTP/WebSocket) when on the same network — always available, no cloud dependency.
- **FR-3** Device SHOULD provide **optional cloud connectivity** for remote access, push
  notifications, and cloud log storage (subscription tier). Loss of cloud MUST NOT affect local
  scheduling or local control.
- **FR-4** All schedule execution MUST continue uninterrupted during WiFi/cloud outages.

### 2.2 Time Management
- **FR-5** On every boot, device MUST sync time from **NTP** when network is available.
- **FR-6** RTC (DS3231) MUST be updated from NTP after a successful sync and used as the
  authoritative clock when offline.
- **FR-7** If both NTP and RTC fail, device MUST raise an alert (see Notifications) and fall back to
  a `millis()`-based soft clock so scheduling degrades gracefully instead of halting.
- **FR-7a** Device MUST apply a **timezone** so schedules run in local time. The timezone is a
  **POSIX TZ string** (DST-aware), provisioned at onboarding and persisted in NVS, defaulting to
  `UTC0`. NTP delivers UTC; the device converts to local time via the configured TZ.

### 2.3 WiFi Provisioning
- **FR-8** First-time WiFi setup MUST use **BLE provisioning** (configure SSID/password via phone).
- **FR-9** Credentials MUST be stored securely in NVS and survive reboots.
- **FR-10** Device MUST support re-provisioning (e.g., factory reset / "forget WiFi") without
  reflashing.

### 2.4 Relay Channel Control (Generic, configurable)
- **FR-11** Outputs MUST be modeled as **N generic relay channels** (not hardcoded appliances).
  End-user-configurable per channel: name, enabled flag, and schedule. (Polarity is NOT
  user-editable — see FR-12.)
- **FR-12** Each channel has a **relay polarity** (active-high vs active-low) determined by the
  relay hardware fitted at manufacture. It is **provisioned once** — stored server-side and set on
  the device at customer registration, persisted in NVS — and is immutable at runtime (not part of
  the schedule CRUD surface). The same firmware thus works across relay hardware variants.
- **FR-13** On boot/restart, channels MUST be driven to their **schedule-correct state immediately**
  and atomically, avoiding output flicker/glitch (no transient toggle during init).
- **FR-14** Each channel has its **own schedule** of time windows (start/end in
  minutes-since-midnight). The window count is **user-defined** via the app, defaulting to **1**
  window on a new channel and capped at **8 windows per channel** (bounded embedded storage).
  A per-channel **`inverted` flag** flips meaning: normal = ON during windows; inverted = ON all day
  *except* during windows (e.g. pump). A single window MAY **cross midnight** (start > end wraps).
- **FR-15** Schedule evaluation MUST be deterministic and based on local time.
- **FR-16** **Electrical load distribution:** when multiple channels must change state at the same
  time, the firmware MUST **stagger** the relay transitions (apply them spaced apart, never all in
  the same instant) to limit inrush current. Staggering MUST remain non-blocking and MUST NOT cause
  flicker on any individual channel (each channel moves directly to its correct state).

> Default factory config preserves the legacy behavior:
> - **Light** 10:30–14:30, 17:00–20:00
> - **CO₂** 7:30–13:30, 14:30–20:30
> - **Heater** 0:00–4:00, 14:31–16:31
> - **Pump/Motor** ON 24h except 9:30–10:30, 14:00–15:00
> These windows are intentional and must remain the seeded defaults.

### 2.5 Temperature Monitoring
- **FR-17** Device MUST read water temperature from a DS18B20 (OneWire).
- **FR-18** The sensor address MUST be discovered once and **cached** (single fixed sensor); reads
  MUST NOT re-run `search()` every cycle.
- **FR-19** Every reading MUST be **CRC-validated**; invalid readings and the `-127.0` sentinel
  (loose/disconnected sensor) MUST be rejected and not displayed/logged as real data.
- **FR-20** Temperature MUST be read on a sensible interval (a few seconds), using **non-blocking**
  conversion (start conversion, read result on a later cycle — no blocking `delay(750)`).
- **FR-21** Persistent sensor fault (repeated invalid reads) MUST raise a sensor-fault alert.

### 2.6 Extreme-Temperature Safety & Alerts
- **FR-22** Configurable thresholds with **hardcoded defaults (low = 20 °C, high = 31 °C)** that the
  user MAY modify via the app; modified values MUST persist in NVS (non-volatile) and survive
  reboots.
- **FR-23** When temperature crosses a threshold (or sensor fault occurs), device MUST:
  1. Trigger the **buzzer + app notification** (local + push if cloud-connected), and
  2. Take a **safety action**: de-energize every channel flagged as a **heater-cut target**.
- **FR-24** The **heater-cut target** is chosen **per channel during onboarding** (a provisioned
  `cutOnOverTemp` flag) and persisted in NVS. A persistent **sensor fault** MUST also trigger the
  cut (temperature can no longer be verified → fail safe).
- **FR-25** Alerts MUST clear/auto-recover when temperature returns to the safe band, with
  hysteresis to avoid flapping.

### 2.7 Notifications / Alerting (shared mechanism)
- **FR-26** A single reusable alert manager MUST handle all alert conditions (RTC failure, NTP
  failure, sensor fault, extreme high/low temp). It MUST expose the active alert set so the app/API
  can notify the user (FR-23).
- **FR-27** The buzzer MUST signal alerts with a **criticality-distinct pattern** (e.g. number of
  short beeps and/or beep duration scales with severity), non-blocking.
- **FR-28** Alerts MUST NOT trap the device in an unrecoverable loop (the legacy
  `while(true) buzzer HIGH` is removed); the main loop and watchdog MUST keep running.

### 2.8 Display (OLED SSD1306 128×32)
- **FR-27** Display MUST show time/date, channel states, and temperature in a **clean, even
  rotation**.
- **FR-28** Display MUST show connection/provisioning status (WiFi/cloud/BLE) and active alerts.

### 2.9 Logging
- **FR-29** Logging MUST be **on-demand**: the device records/streams logs only when the app
  requests them (no continuous flood). No always-on serial spam in production.
- **FR-30** **Premium tier:** logs MAY be stored locally on an **SD card** when hardware is present.
- **FR-31** **Subscription tier:** logs MAY be uploaded to **cloud storage** for history/analytics.
- **FR-32** Basic tier needs no persistent log storage.

### 2.10 Configuration & Web UI App
- **FR-33** A companion **Web UI app** MUST support **CRUD** on schedules and channel config,
  login, and device connection/pairing.
- **FR-34** Configuration changes MUST persist on-device (NVS) and apply without reflashing.
- **FR-35** The device MUST expose a versioned local API (HTTP/WebSocket) for the app; the same
  operations SHOULD be reachable via cloud when subscribed.

### 2.11 Authentication
- **FR-36** Auth MUST use a **cloud account** (email/password) that can manage **multiple devices**
  per user.
- **FR-37** Local-network control MUST remain possible via a device-paired credential/token so the
  app works on LAN even if the cloud is unreachable.

### 2.12 OTA Updates
- **FR-38** Device MUST support **OTA firmware updates**.
- **FR-39** OTA images MUST be **integrity- and authenticity-verified** (signed) before being
  applied; failed/invalid updates MUST roll back to the previous working firmware.

---

## 3. Non-Functional Requirements

### 3.1 Reliability & Safety
- **NFR-1** No blocking `delay()` in the control path; scheduling and I/O MUST be non-blocking.
- **NFR-2** Hardware **watchdog** MUST be enabled; the firmware must never hang indefinitely.
- **NFR-3** Outputs MUST fail to a safe, defined state on fault/restart (no flicker — see FR-13).
- **NFR-4** Recoverable faults (sensor, NTP, WiFi) MUST self-heal without a manual reboot.

### 3.2 Security (commercial product)
- **NFR-5** WiFi credentials, device tokens, and cloud secrets MUST be stored securely (NVS,
  ideally encrypted) — never hardcoded in source.
- **NFR-6** All cloud/app communication MUST be encrypted (TLS); local API SHOULD be authenticated.
- **NFR-7** OTA, boot, and flash integrity MUST follow secure-boot / signed-image practices.
- **NFR-8** All external input (app/cloud/BLE payloads, schedules) MUST be validated and bounded
  (guard against buffer/integer overflow and injection).

### 3.3 Code Quality / Resource Use (per repo instruction set)
- **NFR-9** Replace heap-fragmenting `String` and unsafe format usage (e.g.
  `snprintf(buf, 20, str.c_str())`) with bounded `snprintf` into fixed `char[]` buffers using
  `sizeof`. ASCII only on the OLD path (no multibyte `°C` in C-string buffers).
- **NFR-10** Remove dead code (`heaterTimer`, unused `heaterBalance`/return value, dead display
  branch).
- **NFR-11** Separate **logical state** (on/off) from **electrical polarity** (NO/NC) — do not
  conflate relay level with intent.
- **NFR-12** Follow ESP32 memory, runtime, storage, and secure-SDLC instruction files in
  [`.github/instructions/`](../.github/instructions/).

---

## 4. Hardware Summary

| Component | Part | Interface | Notes |
|-----------|------|-----------|-------|
| MCU | ESP32 (4 MB or 8 MB flash) | — | WiFi + BLE; flash SKU sets feature set (HR-1) |
| RTC | DS3231 | I²C | Temp-compensated, NTP-synced, offline authority |
| Temp sensor | DS18B20 | OneWire | CRC-validated, fixed address |
| Display | SSD1306 128×32 | I²C | Status + rotation |
| Relays | N channels | GPIO | NO/NC configurable per channel |
| Buzzer | — | GPIO | Shared alert mechanism |
| SD card | optional | SPI | Premium tier only |

### 4.1 Hardware Requirements

- **HR-1 Flash:** Two supported flash profiles (see *Flash SKUs* above):
  - **4 MB ("Basic" SKU):** dual ~1664 KB OTA app slots + NVS + tiny filesystem + coredump
    ([`partitions_4mb.csv`](../embedded/partitions_4mb.csv)). Requires **cloud and persistent logging
    compiled out** (`FT_FEATURE_CLOUD=0`, `FT_FEATURE_SD_LOG=0`) so the image fits both OTA slots.
  - **8 MB ("Full" SKU):** dual ~3 MB OTA app slots + NVS + ~1.9 MB filesystem + coredump
    ([`partitions_8mb.csv`](../embedded/partitions_8mb.csv)). Required for cloud TLS + persistent logging.
  - **Signed OTA with rollback (FR-38/39) is mandatory on BOTH** — never ship a non-patchable fleet.
- **HR-2 PSRAM:** Not required for the firmware feature set; if a module includes PSRAM it MAY be
  used for TLS/log buffers but MUST NOT be a hard dependency.


---

## 5. Migration Notes (from `esp32/main.ino`)

| Legacy behavior | v2 change |
|-----------------|-----------|
| Pure Arduino, blocking `delay()`, 9600 serial flood | Non-blocking, on-demand logging |
| Hardcoded 4 appliances | Generic N configurable channels |
| Active-low relays only (`ON=false`) | Per-channel NO/NC polarity |
| `heaterTimer` / `heaterBalance` dead code | Removed |
| Time-based heater, temp unused | Schedule + extreme-temp alert/safety cutoff (no closed-loop control) |
| `ds.search()` every read, blocking 750 ms | Cached address, non-blocking conversion |
| No CRC check, `-127.0` shown as data | CRC validation, fault rejection + alert |
| `while(true)` buzzer on RTC fail | Reusable non-blocking alert + millis fallback clock |
| `snprintf(buf, 20, str.c_str())`, `String`, `°C` | Bounded `snprintf` + `char[]`, ASCII |
| No WiFi/NTP/OTA/BLE | WiFi + NTP-on-boot + signed OTA + BLE provisioning |
| Local config only | Web UI CRUD, cloud account, hybrid local/cloud |

---

## 6. Open / Future Items
- Cloud backend architecture and protocol (MQTT vs WebSocket) — to be specified (M3).
- Number/default mapping of relay channels for shipping SKUs.
- Notification delivery provider (push service) for the subscription tier.
- Local API endpoint/message schema detail — to be finalized during M2 API work.

---

## 7. Architecture Decisions (M2 Connectivity)

> Connectivity is strictly **additive**: it must never block or break standalone scheduling
> (FR-1/FR-4). Decisions below are the agreed industry-standard approach for the commercial build.

- **AD-1 Concurrency:** control logic runs in the existing cooperative loop (core 1); networking
  (WiFi/NTP/web server) runs in a dedicated **FreeRTOS task** (core 0). Config writes from the API
  reach the control side through a **single apply-queue** (one hand-off point) rather than ad-hoc
  shared-state locking. WiFi+NTP, having no shared mutable state, may be serviced cooperatively
  until the web server is introduced.
- **AD-2 Timezone:** **POSIX TZ string** via the ESP32 `configTzTime()` path (DST-aware); stored in
  NVS, default `UTC0` (see FR-7a).
- **AD-3 LAN transport:** **REST** (`/api/v1/...`) for CRUD + **WebSocket** for live state push
  (channel states, temperature, alerts) so the app updates without polling.
- **AD-4 LAN security:** **HTTP + per-device bearer token** on the LAN (token issued during BLE
  provisioning, stored in NVS, required on every request). TLS is provided by the **cloud** path
  (M3) for remote access. Accepted tradeoff: no local TLS to avoid on-device cert management;
  mitigated by the token + network boundary (revisit if threat model changes).
- **AD-5 Web server:** `ESPAsyncWebServer` (async, WebSocket-capable).
- **AD-6 BLE provisioning:** a GATT service (scan results, SSID, password, TZ, issued device-token)
  with BLE bonding for link encryption; BLE stack shut down after WiFi is confirmed to reclaim RAM.
- **AD-7 M2 build order:** WiFi+NTP → Local API + auth token → BLE provisioning. (BLE delivers the
  creds/token the other two consume, so their contracts are defined first.)

### 7.1 Local API surface (v1)

All `/api/v1/*` routes require `Authorization: Bearer <device-token>` (constant-time check); the
API is fully locked until a token is provisioned. Plain HTTP on the LAN (AD-4). Writes are validated
(NFR-8) and applied via the control-loop apply-queue (AD-1).

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/healthz` | Unauthenticated liveness probe |
| GET | `/api/v1/info` | Firmware version, config version, channel count |
| GET | `/api/v1/status` | Live: temp, alerts, time source, WiFi state, channel states |
| GET | `/api/v1/config` | Full config (tz, thresholds, per-channel name/enable/schedule + read-only polarity) |
| PUT | `/api/v1/config` | Update user-editable fields only (tz, thresholds, channel name/enable/schedule); polarity/gpio/cut-target stay as provisioned (FR-12) |
| WS | `/api/v1/ws` | Live status push (~1 Hz) |

Auth failures → `401`; invalid input → `400`; transient busy → `503`; accepted writes → `202`.

### 7.2 BLE provisioning (onboarding)

- **NimBLE** GATT service (chosen over Bluedroid for smaller flash/RAM). Advertises **only when
  unprovisioned** (or after an explicit factory reset) to minimize attack surface and free RAM in
  normal operation; **torn down** once provisioning succeeds (AD-6).
- Characteristics require an **encrypted, bonded** link (NFR-5/8): SSID, password, timezone, and an
  apply command are write-only-encrypted; status + the issued device token are read/notify-encrypted.
- On apply: validates inputs, persists WiFi credentials + timezone to NVS, issues the device bearer
  token to the app, then signals the control loop to (re)connect WiFi. All inputs are bounds-checked
  via the same validators as the local API.

### 7.3 Signed OTA + rollback (FR-38/39)

Enabled on **both** flash SKUs. Firmware is delivered via the local API and is authenticated before
it can ever boot:

- **Upload:** `POST /api/v1/ota` (bearer-authorized) streams the image to the **inactive** OTA slot.
  Metadata travels in headers: `X-FW-Version`, `X-FW-SHA256`, `X-FW-Signature`.
- **Integrity + authenticity:** the device computes the image SHA-256 incrementally, checks it
  against `X-FW-SHA256`, then verifies an **ECDSA P-256** signature over that digest against an
  **embedded public key** ([`src/config/ota_pubkey.h`](../embedded/src/config/ota_pubkey.h)). The private key
  stays offline in the release pipeline ([`scripts/sign_firmware.sh`](../embedded/scripts/sign_firmware.sh)).
- **Anti-rollback:** images that are not **strictly newer** than the running version are rejected.
- **Rollback safety:** after activation + reboot, the new image must run healthy for a confirmation
  window before it is marked valid; otherwise the bootloader **rolls back** to the previous slot. No
  fault ever traps the device (FR-28). Boot is flicker-free (FR-13) — schedule state is restored
  from NVS before outputs are enabled.
- Verification failures (`kHashMismatch`, `kBadSignature`, `kNotNewer`, `kTooLarge`, …) abort the
  session and return `400`; the running image is untouched.




