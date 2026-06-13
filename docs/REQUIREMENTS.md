# f-tank — Requirements Specification (v2)

Automated aquarium controller. This document defines requirements for the **next-generation
commercial product**, replacing the legacy Arduino-style sketch in [`esp32/main.ino`](../esp32/main.ino).

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
- **FR-6** RTC (DS1307) MUST be updated from NTP after a successful sync and used as the
  authoritative clock when offline.
- **FR-7** If both NTP and RTC fail, device MUST raise an alert (see Notifications) and fall back to
  a `millis()`-based soft clock so scheduling degrades gracefully instead of halting.

### 2.3 WiFi Provisioning
- **FR-8** First-time WiFi setup MUST use **BLE provisioning** (configure SSID/password via phone).
- **FR-9** Credentials MUST be stored securely in NVS and survive reboots.
- **FR-10** Device MUST support re-provisioning (e.g., factory reset / "forget WiFi") without
  reflashing.

### 2.4 Relay Channel Control (Generic, configurable)
- **FR-11** Outputs MUST be modeled as **N generic relay channels** (not hardcoded appliances).
  Each channel is user-configurable: name, enabled flag, schedule, and relay polarity.
- **FR-12** Each channel MUST support a **relay polarity / type** setting (Normally-Open vs
  Normally-Closed, i.e. active-high vs active-low) so the same firmware works across relay hardware.
- **FR-13** On boot/restart, channels MUST be driven to their **schedule-correct state immediately**
  and atomically, avoiding output flicker/glitch (no transient toggle during init).
- **FR-14** Each channel schedule MUST support multiple ON windows per day (start/end in
  minutes-since-midnight), and an inverted "OFF-window" mode (e.g. pump = always on except listed
  windows).
- **FR-15** Schedule evaluation MUST be deterministic and based on local time.

> Default factory config preserves the legacy behavior:
> - **Light** 10:30–14:30, 17:00–20:00
> - **CO₂** 7:30–13:30, 14:30–20:30
> - **Heater** 0:00–4:00, 14:31–16:31
> - **Pump/Motor** ON 24h except 9:30–10:30, 14:00–15:00
> These windows are intentional and must remain the seeded defaults.

### 2.5 Temperature Monitoring
- **FR-16** Device MUST read water temperature from a DS18B20 (OneWire).
- **FR-17** The sensor address MUST be discovered once and **cached** (single fixed sensor); reads
  MUST NOT re-run `search()` every cycle.
- **FR-18** Every reading MUST be **CRC-validated**; invalid readings and the `-127.0` sentinel
  (loose/disconnected sensor) MUST be rejected and not displayed/logged as real data.
- **FR-19** Temperature MUST be read on a sensible interval (a few seconds), using **non-blocking**
  conversion (start conversion, read result on a later cycle — no blocking `delay(750)`).
- **FR-20** Persistent sensor fault (repeated invalid reads) MUST raise a sensor-fault alert.

### 2.6 Extreme-Temperature Safety & Alerts
- **FR-21** Configurable thresholds. **Defaults: low = 20 °C, high = 31 °C.**
- **FR-22** When temperature crosses a threshold (or sensor fault occurs), device MUST:
  1. Trigger the **buzzer + notification** (local + push if cloud-connected), and
  2. Take a **safety action**: cut the heater channel relay on a high-temp/fault condition.
- **FR-23** Alerts MUST clear/auto-recover when temperature returns to the safe band, with
  hysteresis to avoid flapping.

### 2.7 Notifications / Alerting (shared mechanism)
- **FR-24** A single reusable notification routine MUST handle all alert conditions
  (RTC failure, NTP failure, sensor fault, extreme temp), driving the buzzer pattern and, when
  connected, sending an app/push notification.
- **FR-25** Alerts MUST NOT trap the device in an unrecoverable loop (the legacy
  `while(true) buzzer HIGH` is removed); the main loop and watchdog MUST keep running.

### 2.8 Display (OLED SSD1306 128×32)
- **FR-26** Display MUST show time/date, channel states, and temperature in a **clean, even
  rotation**.
- **FR-27** Display MUST show connection/provisioning status (WiFi/cloud/BLE) and active alerts.

### 2.9 Logging
- **FR-28** Logging MUST be **on-demand**: the device records/streams logs only when the app
  requests them (no continuous flood). No always-on serial spam in production.
- **FR-29** **Premium tier:** logs MAY be stored locally on an **SD card** when hardware is present.
- **FR-30** **Subscription tier:** logs MAY be uploaded to **cloud storage** for history/analytics.
- **FR-31** Basic tier needs no persistent log storage.

### 2.10 Configuration & Web UI App
- **FR-32** A companion **Web UI app** MUST support **CRUD** on schedules and channel config,
  login, and device connection/pairing.
- **FR-33** Configuration changes MUST persist on-device (NVS) and apply without reflashing.
- **FR-34** The device MUST expose a versioned local API (HTTP/WebSocket) for the app; the same
  operations SHOULD be reachable via cloud when subscribed.

### 2.11 Authentication
- **FR-35** Auth MUST use a **cloud account** (email/password) that can manage **multiple devices**
  per user.
- **FR-36** Local-network control MUST remain possible via a device-paired credential/token so the
  app works on LAN even if the cloud is unreachable.

### 2.12 OTA Updates
- **FR-37** Device MUST support **OTA firmware updates**.
- **FR-38** OTA images MUST be **integrity- and authenticity-verified** (signed) before being
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
| MCU | ESP32 | — | WiFi + BLE |
| RTC | DS1307 | I²C | NTP-synced, offline authority |
| Temp sensor | DS18B20 | OneWire | CRC-validated, fixed address |
| Display | SSD1306 128×32 | I²C | Status + rotation |
| Relays | N channels | GPIO | NO/NC configurable per channel |
| Buzzer | — | GPIO | Shared alert mechanism |
| SD card | optional | SPI | Premium tier only |

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
- Exact local API surface (HTTP routes / WebSocket message schema) — to be specified.
- Cloud backend architecture and protocol (MQTT vs WebSocket) — to be specified.
- Number/default mapping of relay channels for shipping SKUs.
- Notification delivery provider (push service) for the subscription tier.
- Buzzer alert patterns per alert type (distinct tones).
