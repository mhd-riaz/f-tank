# f-tank — Copilot Instructions

Automated aquarium controller firmware for **ESP32**. Drives N relay-controlled appliances on
time-based schedules, monitors water temperature, and is configured via a companion Web UI app
(local + optional cloud). This repo is the **firmware**.

## Start here — read before acting

1. **[docs/REQUIREMENTS.md](../docs/REQUIREMENTS.md)** — the authoritative spec for the v2
   commercial product. Every change MUST align with it. Requirements are referenced by ID
   (`FR-*`, `NFR-*`); cite them when implementing or discussing features.
2. **[esp32/main.ino](../esp32/main.ino)** — legacy v1 sketch. Treat as reference only; it has known
   issues being designed out (see REQUIREMENTS §5 Migration Notes). Do not copy its anti-patterns.
3. **`.github/instructions/`** — mandatory engineering rules (auto-applied to `*.{cpp,c,h,hpp,ino}`).
   Always follow them when touching firmware.

## Instruction files — when each applies

| File                                                                              | Use when you are…                                                                           |
| --------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| [code-quality](instructions/code-quality.instructions.md)                         | writing/refactoring any firmware — naming, structure, error handling, testability           |
| [memory-optimization](instructions/memory-optimization.instructions.md)           | allocating memory, sizing buffers, handling strings, designing structs (~300KB usable heap) |
| [runtime-optimization](instructions/runtime-optimization.instructions.md)         | writing loops, ISRs, sensor polling, network handlers, timing-sensitive code                |
| [secure-sdlc](instructions/secure-sdlc.instructions.md)                           | network code, OTA, boot, secrets handling                                                   |
| [storage-optimization](instructions/storage-optimization.instructions.md)         | writing to flash, NVS, Preferences, SPIFFS/LittleFS, SD card                                |
| [vulnerability-protection](instructions/vulnerability-protection.instructions.md) | parsing input, building strings/topics, implementing protocols                              |

## Architecture (target v2)

- **Platform:** ESP32, WiFi + BLE. Standalone scheduling works with no network.
- **Connectivity:** Hybrid — local LAN API (HTTP/WebSocket) always available; cloud optional and
  must never block scheduling or local control.
- **Time:** NTP sync on every boot → updates DS1307 RTC (offline authority) → `millis()` soft-clock
  fallback if both fail.
- **Channels:** Generic N configurable relay channels. Each has name, enable flag, multi-window
  schedule (minutes-since-midnight, supports inverted OFF-windows), and **NO/NC polarity**.
- **Temp:** DS18B20, cached address, non-blocking conversion, CRC-validated, `-127.0` = fault.
- **Safety:** Extreme temp / sensor fault → reusable alert (buzzer + notification) **and** cut heater
  relay. Defaults: low 20 °C, high 31 °C, with hysteresis.
- **Config:** Web UI does CRUD over a versioned API; persisted in NVS; no reflash to change.
- **Updates:** Signed OTA with rollback. Provisioning via BLE.

## Hard rules (derived from spec + instructions)

- **No blocking `delay()`** in the control path. Scheduling/I/O must be non-blocking (NFR-1).
- **No `while(true)` trap** on faults — always keep the loop + watchdog alive (FR-25, NFR-2).
- **Separate logical state (on/off) from electrical polarity (NO/NC)** — never conflate relay level
  with intent (NFR-11).
- **No flicker on boot/restart** — drive channels to schedule-correct state atomically (FR-13).
- **Strings:** bounded `snprintf` into fixed `char[]` with `sizeof`. No heap-fragmenting `String`,
  no runtime string as format arg, ASCII only in C-string buffers (NFR-9).
- **Validate all external input** (app/cloud/BLE/schedules); guard buffer/integer overflow (NFR-8).
- **Never hardcode secrets** (WiFi creds, tokens); store securely in NVS (NFR-5).
- **Don't reintroduce removed dead code** (`heaterTimer`, `heaterBalance`, closed-loop temp control).

## Project tiers (affects features)

- **Basic:** local control, scheduling, on-demand logs, temp alerts.
- **Premium:** + SD-card local logging.
- **Subscription:** + cloud logging, remote access.

Logging is **on-demand only** — record/stream when the app requests, never continuous serial spam.

## Working agreements

- Cite requirement IDs when implementing features.
- If a request conflicts with REQUIREMENTS.md, flag it before proceeding.
- Keep changes minimal and aligned with the spec; don't add unrequested features.
- Open/undecided design areas live in REQUIREMENTS §6 — confirm before assuming them.
