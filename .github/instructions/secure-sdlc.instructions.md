---
description: "Use when writing, reviewing, or modifying any firmware code. Enforces secure software development lifecycle practices for ESP32 embedded systems — threat modeling, input validation, secure boot, OTA integrity, and pre-commit security checks."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Secure Software Development Lifecycle (SDLC) — ESP32 Firmware

## Threat Modeling

- Identify attack surfaces before implementing any network-facing feature (WiFi, MQTT, HTTP, BLE)
- Document trust boundaries: sensor inputs, serial console, network interfaces, flash storage
- Assume all external inputs (serial, network, sensor ADC) are untrusted

## Secure Boot & Flash Integrity

- Enable ESP32 secure boot v2 in production builds
- Use flash encryption for sensitive credentials and configuration
- Never store plaintext WiFi passwords, API keys, or tokens in source — use NVS encrypted partition
- Validate firmware signatures during OTA updates before applying

## Input Validation at System Boundaries

- Validate all sensor readings against physical plausible ranges before processing
- Sanitize and length-check all incoming network payloads (MQTT messages, HTTP params)
- Reject malformed packets early — never pass raw network data to business logic

## Secrets Management

- Use `#include` for credentials from a git-ignored `secrets.h` — never hardcode
- Rotate credentials if they appear in version control history
- Use hardware-backed key storage (ESP32 eFuse) for production secrets

## OTA Update Security

- Require HTTPS with certificate pinning for OTA download endpoints
- Verify firmware hash (SHA-256 minimum) before flashing
- Implement rollback mechanism — if new firmware fails health check, revert to last-known-good

## Pre-Commit Checks

- No `TODO: fix security` or `// HACK` in committed code without a linked issue
- All network-facing functions must have an associated threat note in comments
- Compiler warnings treated as errors (`-Werror`) — no suppression without justification
