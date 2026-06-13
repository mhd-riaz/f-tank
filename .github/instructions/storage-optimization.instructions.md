---
description: "Apply when writing to flash, NVS, Preferences, SPIFFS/LittleFS, or SD card. Enforces ESP32 storage efficiency — flash wear leveling, write batching, partition planning, compact binary formats, and filesystem discipline."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Storage Optimization — ESP32 Firmware

> Goal: protect flash endurance. Every avoidable write shortens device life. Flash sectors survive only ~10k–100k writes.

## NEVER (reject the code if violated)

- NEVER write to flash/NVS on every sensor reading or inside a tight loop.
- NEVER store time-series data in NVS — use a filesystem or SD card.
- NEVER write config unless the value actually changed (dirty-flag pattern).
- NEVER store sensor logs as JSON/CSV text on internal flash — use packed binary.

## ALWAYS

- ALWAYS batch writes: accumulate in RAM, flush on interval (e.g. every 5 min) or on meaningful change.
- ALWAYS use NVS/Preferences for small config/calibration (it wear-levels internally); group keys by namespace (`"wifi"`, `"sensors"`).
- ALWAYS prefer LittleFS over SPIFFS (better wear leveling, directories, power-loss resilience).
- ALWAYS budget endurance: `writes_per_day × 365 × target_years < flash_endurance`.
- ALWAYS rotate logs (max file count / by date / by size) and delete old files proactively.

## Write-Only-On-Change Pattern

```cpp
// WRONG — flash write every loop iteration
void loop() {
  preferences.putFloat("temp", currentTemp);  // ~every 100ms!
}

// CORRECT — write only on meaningful change
if (fabs(currentTemp - lastSavedTemp) > 0.5f) {
  preferences.putFloat("temp", currentTemp);
  lastSavedTemp = currentTemp;
}
```

## Compact Binary Storage

- Pack records: `timestamp(4B) + temp(2B) + pH(2B) = 8B` vs 40+ B as text.
- Store fixed-point integers, not floats: `temp_x100 = 2534` instead of `25.34f`.
- Apply simple delta/RLE encoding to repetitive series before writing.

## Partition Planning

- Size the app partition to actual firmware size — don't keep the default 1.5MB if you use 800KB.
- Reserve dual OTA banks (app0 + app1) for safe updates.
- Size SPIFFS/LittleFS to real need + ~20% headroom; document layout in `partitions.csv`.

## Filesystem Discipline (LittleFS)

- Keep filenames short; limit file count (each file carries ~256B metadata).
- Flush/sync regularly and close files safely to survive power loss.

## SD Card (high-frequency logging)

- Prefer SD over internal flash for high-rate logs; use 512-byte-aligned buffered writes.
- Rotate files by date/size to limit single-file corruption loss.

## Self-check before finishing

1. No per-reading/per-loop flash writes; writes batched or change-gated?
2. Time-series on filesystem/SD, config in NVS by namespace?
3. Logs stored as packed binary/fixed-point and rotated?
4. Partition sizes match real needs with OTA banks reserved?
