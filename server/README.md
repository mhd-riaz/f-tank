# f-tank server (`server/`)

Cloud backend for f-tank. **Planned / not yet implemented.**

Responsibilities (per [`docs/REQUIREMENTS.md`](../docs/REQUIREMENTS.md)):

- MQTT broker integration (device telemetry in, config/commands out) — see spec §7 Architecture
  Decisions and the firmware `cloud/` module for the device-side contract.
- REST API for the web app (auth, device registry, schedules, logs).
- Device identity (X.509 mutual TLS) and subscription state.
- Log storage (Subscription tier).

## Contract

The device ↔ cloud contract is defined in [`docs/REQUIREMENTS.md`](../docs/REQUIREMENTS.md):
MQTT topics are `ftank/<deviceId>/{telemetry,status,config/state,config/set,cmd}`; config payloads
mirror the local API config JSON. Keep this server and the firmware in sync via that spec.

## Rollout plan

1. Start against a **self-hosted MQTT broker** (Mosquitto/EMQX) on the home LAN with a private CA.
2. Move to a managed broker (e.g. AWS IoT Core, Mumbai `ap-south-1`) for beta — firmware only needs
   re-provisioned endpoint + certs, no code change.
