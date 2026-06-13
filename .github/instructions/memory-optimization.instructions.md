---
description: "Use when allocating memory, creating buffers, using strings, or designing data structures. Enforces memory-efficient patterns for ESP32 (520KB SRAM, ~300KB usable heap) — stack discipline, heap hygiene, and fragmentation avoidance."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Memory Optimization — ESP32 Firmware

## Heap Budget Awareness

- ESP32 has ~300KB usable heap — always check `ESP.getFreeHeap()` after major allocations
- Set a heap floor (e.g., 30KB minimum free) — if crossed, disable non-essential features
- Log heap usage at boot and periodically for leak detection

## Stack Discipline

- Default FreeRTOS task stack: 4KB–8KB — keep local variables small
- Never declare large arrays on the stack: `char buf[4096]` on stack = stack overflow risk
- Use `uxTaskGetStackHighWaterMark()` to validate stack sizing during development
- Prefer static/global buffers for large fixed-size data, allocated once at init

## Avoid Dynamic Allocation in Loops

- Never call `malloc`/`new` inside `loop()` or periodic tasks — pre-allocate at startup
- If dynamic allocation is unavoidable, pair every `malloc` with `free` in the same scope
- Prefer pool allocators or ring buffers for repeated same-size allocations

```cpp
// WRONG — heap fragmentation over time
void loop() {
  char* msg = (char*)malloc(128);
  // ... use msg ...
  free(msg);
}

// CORRECT — pre-allocated buffer
static char msgBuffer[128];
void loop() {
  // ... use msgBuffer ...
}
```

## String Handling

- Avoid `String` class (Arduino) in production — causes heap fragmentation
- Use fixed `char[]` buffers with `snprintf` for string formatting
- For JSON construction, use `ArduinoJson` with `StaticJsonDocument` (stack) over `DynamicJsonDocument` (heap)
- Set JSON document size precisely using the ArduinoJson Assistant

## Data Structure Sizing

- Use smallest sufficient integer types: `uint8_t` for values 0–255, `uint16_t` for 0–65535
- Pack structs with `__attribute__((packed))` when alignment padding wastes RAM
- Use bitfields for boolean flags: `struct { uint8_t pumpOn:1; uint8_t heaterOn:1; };`
- Store lookup tables in flash with `PROGMEM` / `const` (ESP32 maps flash to address space)

## PSRAM Usage (if available)

- Offload large buffers (display framebuffers, data logs) to PSRAM: `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`
- Keep latency-critical data in internal SRAM
- Check PSRAM availability at runtime: `ESP.getPsramSize()`

## Leak Detection

- Monitor `ESP.getFreeHeap()` over time — any consistent downward trend indicates a leak
- Use `heap_caps_check_integrity_all()` in debug builds to catch corruption early
- Enable heap tracing (`heap_trace_start`) during development for allocation tracking
