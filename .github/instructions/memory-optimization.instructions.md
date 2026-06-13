---
description: "Apply when allocating memory, sizing buffers, handling strings, or designing structs/data layouts on ESP32 (~520KB SRAM, ~300KB usable heap). Enforces stack discipline, heap hygiene, and fragmentation avoidance."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Memory Optimization — ESP32 Firmware

> Goal: stable long-running heap with no fragmentation creep. The enemy is repeated dynamic allocation.

## NEVER (reject the code if violated)

- NEVER call `malloc`/`new`/`String` growth inside `loop()` or any periodic task — pre-allocate at init.
- NEVER declare large arrays on the stack (e.g. `char buf[4096]`) — task stacks are 4–8KB.
- NEVER use the Arduino `String` class in production paths — it fragments the heap; use `char[]` + `snprintf`.
- NEVER `malloc` without a guaranteed matching `free` in the same scope.

## ALWAYS

- ALWAYS budget the heap: keep a floor (~30KB free); below it, disable non-essential features instead of crashing.
- ALWAYS use the smallest sufficient integer type (`uint8_t` 0–255, `uint16_t` 0–65535).
- ALWAYS pre-allocate large fixed-size buffers once as `static`/global.
- ALWAYS size `ArduinoJson` with `StaticJsonDocument` (stack) over `DynamicJsonDocument` (heap); compute size via the ArduinoJson Assistant.
- ALWAYS store constant lookup tables in flash with `PROGMEM`/`const`.

## Allocation Pattern

```cpp
// WRONG — heap fragmentation over time
void loop() {
  char* msg = (char*)malloc(128);
  // ... use msg ...
  free(msg);
}

// CORRECT — pre-allocated, reused buffer
static char msgBuffer[128];
void loop() {
  snprintf(msgBuffer, sizeof(msgBuffer), "...");
}
```

## Stack Discipline

- Keep task locals small; validate sizing with `uxTaskGetStackHighWaterMark()` during development.
- Prefer static/global storage for anything large.

## Struct Packing

- Pack flags into bitfields: `struct { uint8_t pumpOn:1; uint8_t heaterOn:1; };`.
- Use `__attribute__((packed))` only when padding genuinely wastes RAM (watch unaligned-access cost).

## PSRAM (if present)

- Offload large buffers (framebuffers, logs) to PSRAM: `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`.
- Keep latency-critical data in internal SRAM. Check availability with `ESP.getPsramSize()`.

## Leak Detection

- Log `ESP.getFreeHeap()` at boot and periodically — any steady downward trend is a leak.
- In debug builds use `heap_caps_check_integrity_all()` and `heap_trace_start` to catch corruption/allocations.

## Self-check before finishing

1. Zero dynamic allocation inside loops/periodic tasks?
2. No large stack arrays; no production `String` usage?
3. Smallest integer types and pre-allocated buffers used?
4. Free-heap logging in place to surface leaks?
