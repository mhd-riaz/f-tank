---
description: "Use when writing task loops, ISRs, sensor polling, network handlers, or control algorithms. Enforces runtime-efficient patterns for ESP32 — CPU cycle awareness, task scheduling, power management, and real-time responsiveness."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Runtime Optimization — ESP32 Firmware

## CPU Cycle Awareness

- ESP32 dual-core at 240MHz — use both cores for parallelism (Core 0: network, Core 1: sensors/control)
- Avoid floating-point math in tight loops — ESP32 has no FPU hardware for double, only single-precision float
- Use integer arithmetic and fixed-point where possible in ISRs and high-frequency paths
- Replace `pow()`, `sqrt()` with lookup tables or linear approximations for non-critical calculations

## Task Scheduling (FreeRTOS)

- Use `vTaskDelay()` not `delay()` — yields CPU to other tasks instead of busy-waiting
- Set task priorities based on criticality: safety checks > control loops > logging > display
- Pin time-critical tasks to a specific core with `xTaskCreatePinnedToCore()`
- Keep ISRs under 5μs — defer work to tasks via queues or semaphores

```cpp
// WRONG — blocks entire core
void loop() {
  delay(1000);  // busy-wait, no yielding
  readSensors();
}

// CORRECT — cooperative multitasking
void sensorTask(void* param) {
  for (;;) {
    readSensors();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

## Polling vs Interrupts

- Use hardware interrupts for edge-triggered events (flow sensors, buttons, alarms)
- Use timer-based polling for periodic reads (temperature, pH — no faster than physical response time)
- Never poll faster than sensor update rate — wastes CPU cycles for stale data
- Debounce interrupt-driven inputs in software (50–100ms window)

## Network Efficiency

- Use non-blocking network calls — never `while(!client.available())` without timeout
- Batch telemetry: send one MQTT message with multiple readings vs one per sensor
- Use QoS 0 for frequent sensor data, QoS 1 only for commands/alerts
- Keep MQTT payloads compact: binary or msgpack over verbose JSON for high-frequency data

## Power Management

- Use `esp_sleep_enable_timer_wakeup()` for periodic deep sleep between readings if battery-powered
- Disable unused peripherals: `WiFi.mode(WIFI_OFF)` when not transmitting
- Reduce CPU frequency during idle periods: `setCpuFrequencyMhz(80)` for light tasks
- Use light sleep for sub-second wake intervals, deep sleep for minutes+

## Control Loop Timing

- Use `micros()` for precise loop timing — never rely on `delay()` for control frequency
- Implement fixed-timestep control loops with drift compensation
- PID loops: compute dt from actual elapsed time, not assumed constant
- Log loop overruns — if a cycle takes longer than budget, flag it

```cpp
static uint32_t lastRunUs = 0;
void controlLoop() {
  uint32_t now = micros();
  uint32_t dt = now - lastRunUs;
  if (dt < CONTROL_PERIOD_US) return;  // not time yet
  lastRunUs = now;
  // ... PID computation using actual dt ...
}
```

## Avoid Common Performance Traps

- `Serial.print()` in production loops: UART is slow (115200 baud ≈ 1ms per 11 chars) — use conditional logging
- Repeated WiFi reconnection attempts without backoff: burns CPU and floods the network
- String concatenation in loops: each `+` allocates and copies — use `snprintf` to fixed buffer
- Recursive functions on embedded: stack depth is limited — convert to iterative
