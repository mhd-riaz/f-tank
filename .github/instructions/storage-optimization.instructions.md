---
description: "Use when writing to flash, NVS, SPIFFS/LittleFS, or SD card. Enforces storage-efficient patterns for ESP32 — flash wear leveling, partition planning, data compression, and filesystem discipline."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Storage Optimization — ESP32 Firmware

## Flash Wear Awareness

- ESP32 flash endurance: ~10,000–100,000 write cycles per sector
- Never write to flash in a tight loop or on every sensor reading
- Batch writes: accumulate data in RAM, flush periodically (e.g., every 5 minutes or on threshold change)
- Use NVS (Non-Volatile Storage) for key-value config — it handles wear leveling internally

## Write Frequency Discipline

- Calculate worst-case write frequency: `writes_per_day × 365 × target_years < flash_endurance`
- For high-frequency data (sensor logs), use circular buffer in RAM and flush to flash on intervals
- Implement dirty-flag pattern: only write config when values actually change

```cpp
// WRONG — writes every loop iteration
void loop() {
  preferences.putFloat("temp", currentTemp);  // flash write every 100ms!
}

// CORRECT — write only on meaningful change
if (abs(currentTemp - lastSavedTemp) > 0.5) {
  preferences.putFloat("temp", currentTemp);
  lastSavedTemp = currentTemp;
}
```

## Partition Table Planning

- Allocate partition sizes based on actual needs — don't use default 1.5MB app partition if firmware is 800KB
- Reserve OTA partition for dual-bank updates (app0 + app1)
- Size SPIFFS/LittleFS partition for actual data needs + 20% headroom
- Document partition layout in a `partitions.csv` with comments

## Data Format Efficiency

- Use binary formats for sensor logs — not JSON or CSV on flash
- Pack sensor readings into structs: timestamp (4B) + temp (2B) + pH (2B) = 8 bytes vs 40+ bytes as text
- Use fixed-point integers instead of floats for storage: `temp_x100 = 2534` instead of `25.34f`
- Compress repetitive data with simple RLE or delta encoding before writing

## Filesystem Best Practices (LittleFS/SPIFFS)

- Prefer LittleFS over SPIFFS — better wear leveling, directory support, power-loss resilience
- Keep filenames short (8.3 format saves metadata space)
- Limit number of files — each file has metadata overhead (~256 bytes on LittleFS)
- Delete old log files proactively — implement log rotation with max file count

## NVS Usage

- Use NVS for configuration and calibration data (small, infrequent writes)
- Group related keys under namespaces: `"wifi"`, `"sensors"`, `"control"`
- Never store time-series data in NVS — use filesystem or external storage
- Check `nvs_get_stats()` to monitor NVS partition usage

## External Storage (SD Card)

- If logging high-frequency data, prefer SD card over internal flash
- Use buffered writes (512-byte aligned for SD performance)
- Implement safe file closing on power loss — flush and sync regularly
- Rotate log files by date or size to prevent single-file corruption loss
