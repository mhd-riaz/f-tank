# f-tank firmware (`embedded/`)

ESP32 firmware (PlatformIO). Run all `pio` commands from this directory.

## Build & test

```sh
pio run -e esp32_8mb_full      # 8 MB "Full" SKU (cloud + persistent logging + OTA)
pio run -e esp32_4mb_basic     # 4 MB "Basic" SKU (cloud + logging compiled out; OTA kept)
pio test -e native             # hardware-free unit tests
pio check                      # static analysis
```

SKU features are selected by compile-time flags in [`src/config/features.h`](src/config/features.h)
(`FT_FEATURE_CLOUD`, `FT_FEATURE_SD_LOG`, `FT_FEATURE_OTA`). Partition tables:
[`partitions_8mb.csv`](partitions_8mb.csv) / [`partitions_4mb.csv`](partitions_4mb.csv).

## Module map (`src/`)

| Module | Responsibility |
| ------ | -------------- |
| `config/` | `pins.h`, `features.h`, `ota_pubkey.h`, `secrets.h` (git-ignored) |
| `time/` | DS3231 RTC + `millis()` soft-clock fallback; pure `WallClock` math |
| `channel/` | Relay channels; logical state vs NO/NC polarity |
| `schedule/` | Per-channel multi-window scheduler (inverted + cross-midnight) |
| `control/` | `ScheduleRunner` (load-staggered apply), `SafetyController` (temp cutoff) |
| `sensor/` | Non-blocking, CRC-validated DS18B20 driver |
| `alert/` | Reusable alert manager + non-blocking buzzer patterns |
| `display/` | SSD1306 OLED rotation; pure ASCII formatters |
| `storage/` | Versioned, CRC'd NVS config (`ConfigStore`) |
| `network/` | WiFi state machine + NTP; secure credential store |
| `api/` | Local REST + WebSocket, bearer auth, validation, apply-queue brokers |
| `provisioning/` | NimBLE GATT onboarding (WiFi creds + token) |
| `ota/` | Streaming signed OTA (SHA-256 + ECDSA P-256) with rollback |
| `cloud/` | MQTT-over-TLS link (8 MB only; `#if FT_FEATURE_CLOUD`) |

`src/main.cpp` is **wiring only** — it instantiates modules and services them cooperatively
(non-blocking). Feature logic lives in the modules.

## Conventions

Engineering rules are enforced via [`.github/instructions/`](../.github/instructions/) and apply to
all `*.{cpp,c,h,hpp,ino}`. Pure logic is kept hardware-free so it unit-tests under `env:native`.
The authoritative spec is [`docs/REQUIREMENTS.md`](../docs/REQUIREMENTS.md); cite `FR-*`/`NFR-*` IDs.
