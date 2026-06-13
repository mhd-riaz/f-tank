---
description: "Apply when writing task loops, ISRs, sensor polling, network handlers, control/PID algorithms, or power management. Enforces ESP32 runtime efficiency — CPU cycles, FreeRTOS scheduling, non-blocking I/O, and deterministic timing."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Runtime Optimization — ESP32 Firmware

> Goal: cooperative, non-blocking, deterministic timing. No core ever busy-waits; no ISR ever does real work.

## NEVER (reject the code if violated)

- NEVER use `delay()` in tasks/loops — use `vTaskDelay(pdMS_TO_TICKS(ms))` so the CPU yields.
- NEVER do heavy work in an ISR — keep it < 5µs; defer to a task via queue/semaphore.
- NEVER block indefinitely on network/serial reads — every read has a timeout.
- NEVER poll a sensor faster than its physical update rate — it wastes cycles on stale data.

## ALWAYS

- ALWAYS split work across cores: Core 0 for network, Core 1 for sensors/control via `xTaskCreatePinnedToCore()`.
- ALWAYS prioritize tasks by criticality: safety > control > logging > display.
- ALWAYS prefer integer/fixed-point math in hot paths and ISRs (ESP32 FPU is single-precision `float` only; `double` is slow).
- ALWAYS debounce interrupt-driven inputs in software (50–100ms).
- ALWAYS gate verbose logging — `Serial.print()` at 115200 baud is ~1ms per 11 chars.

## Cooperative Task Pattern

```cpp
// WRONG — blocks the entire core
void loop() {
  delay(1000);     // busy-wait, no yielding
  readSensors();
}

// CORRECT — yields to scheduler
void sensorTask(void* param) {
  for (;;) {
    readSensors();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

## Deterministic Control Loops

```cpp
// Fixed-timestep using real elapsed time, not assumed constants
static uint32_t lastRunUs = 0;
void controlLoop() {
  uint32_t now = micros();
  if (now - lastRunUs < CONTROL_PERIOD_US) return;  // not time yet
  uint32_t dt = now - lastRunUs;
  lastRunUs = now;
  // PID uses actual dt; flag/log if dt overruns the budget
}
```

## Polling vs Interrupts

- Edge events (flow sensors, buttons, alarms) → hardware interrupts.
- Periodic reads (temperature, pH) → timer-based polling at the sensor's response rate.

## Network Efficiency

- Batch telemetry into one MQTT message instead of one per sensor.
- QoS 0 for frequent sensor data; QoS 1 only for commands/alerts.
- Keep high-frequency payloads compact (binary/msgpack over verbose JSON).
- Reconnect with exponential backoff — never tight-loop reconnection.

## Power Management (battery builds)

- Deep sleep between readings: `esp_sleep_enable_timer_wakeup()`; light sleep for sub-second intervals.
- Power down idle peripherals (`WiFi.mode(WIFI_OFF)` when not transmitting).
- Lower clock for light work: `setCpuFrequencyMhz(80)`.

## Self-check before finishing

1. No `delay()` in tasks; all reads have timeouts?
2. ISRs < 5µs with work deferred to tasks?
3. Control timing derived from `micros()`/real `dt`?
4. Hot paths use integer/fixed-point and gated logging?
