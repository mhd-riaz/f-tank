# f-tank

Automated aquarium controller — a commercial IoT product. An **ESP32** drives N relay-controlled
appliances (lighting, CO₂, heater, pump, …) on time-based schedules, monitors water temperature,
raises safety alerts, and is configured locally (LAN) or remotely (cloud) from a companion web app.

This is a **monorepo** with three concerns that share one contract:

| Folder | Concern | Status |
| ------ | ------- | ------ |
| [`embedded/`](embedded/) | ESP32 firmware (PlatformIO) | active |
| [`server/`](server/) | Cloud backend (MQTT, REST API, device registry, logs) | planned |
| [`ui/`](ui/) | Web UI app | planned |
| [`docs/`](docs/REQUIREMENTS.md) | **REQUIREMENTS.md** — authoritative shared spec | active |

The API schema, MQTT topics, and config JSON are defined once in
[`docs/REQUIREMENTS.md`](docs/REQUIREMENTS.md); cross-cutting changes update firmware + server + ui
together.

## Firmware quick start

```sh
cd embedded
pio run -e esp32_8mb_full      # build 8 MB "Full" SKU (default)
pio run -e esp32_4mb_basic     # build 4 MB cost-optimized "Basic" SKU
pio test -e native             # run hardware-free unit tests
```

See [`embedded/README.md`](embedded/README.md) for the firmware module map and SKU details.

## License

See [LICENSE](LICENSE).
