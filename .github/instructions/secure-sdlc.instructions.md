---
description: "Apply when writing, reviewing, or modifying any firmware, especially network-facing code, OTA, boot, or secrets handling. Enforces secure SDLC for ESP32 — threat modeling, input validation, secure boot, flash/OTA integrity, and pre-commit checks."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Secure SDLC — ESP32 Firmware

> Goal: treat every external input as hostile and every secret as already-leaked-if-plaintext. Security is a build gate, not a follow-up.

## NEVER (reject the code if violated)

- NEVER hardcode WiFi passwords, API keys, or tokens in source — load from git-ignored `secrets.h` or encrypted NVS.
- NEVER pass raw network/serial/ADC data into business logic without validation.
- NEVER apply an OTA image without verifying its signature and SHA-256 hash first.
- NEVER commit `TODO: fix security` / `// HACK` on a security path without a linked tracking issue.

## ALWAYS

- ALWAYS treat serial, network, BLE, and sensor ADC as untrusted trust boundaries.
- ALWAYS validate sensor readings against physically plausible ranges before use.
- ALWAYS length-check and sanitize incoming payloads (MQTT, HTTP params) and reject malformed packets early.
- ALWAYS enable Secure Boot v2 and flash encryption in production builds.
- ALWAYS treat compiler warnings as errors (`-Werror`); no suppression without written justification.

## Threat Modeling (do this before coding a network feature)

- Enumerate the attack surface (WiFi, MQTT, HTTP, BLE) and document trust boundaries.
- Add a one-line threat note in comments for each network-facing function (what input, what risk, what mitigation).

## Secrets Management

- Store production secrets in hardware-backed storage (ESP32 eFuse) or encrypted NVS.
- If a secret ever entered version control, rotate it — do not just delete it.

## OTA Update Security

- Download over HTTPS with certificate pinning.
- Verify firmware hash (SHA-256 minimum) and signature before flashing.
- Keep dual-bank rollback: if new firmware fails its health check, revert to last-known-good.

## Self-check before finishing

1. No plaintext secrets; secrets sourced from `secrets.h`/encrypted NVS?
2. Every external input validated/length-checked at its boundary?
3. OTA path verifies signature + hash and can roll back?
4. Each network-facing function has a threat note; build is `-Werror` clean?
