# f-tank web UI (`ui/`)

Companion web app for f-tank. **Planned / not yet implemented.**

Responsibilities (per [`docs/REQUIREMENTS.md`](../docs/REQUIREMENTS.md)):

- Onboarding/pairing and login.
- CRUD over channels, schedules, thresholds, and timezone.
- Live status (temperature, channel states, alerts) via WebSocket.
- Works **locally** against the device REST/WebSocket API on the LAN, and **remotely** via the
  cloud backend (Subscription tier).

## Contract

Talks to the device's versioned local API (`/api/v1/...`, bearer token) defined in
[`docs/REQUIREMENTS.md`](../docs/REQUIREMENTS.md) §7.1, and the cloud REST API (planned, `server/`).
Both converge on the same validated config schema — keep the UI forms aligned with that spec.
