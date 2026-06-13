---
description: "Use when writing or reviewing any code. Enforces code quality standards for ESP32 Arduino firmware — naming conventions, structure, static analysis compliance, testability, and maintainability."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Code Quality Standards — ESP32 Firmware

## Naming & Structure

- Use `camelCase` for local variables and functions, `PascalCase` for classes/structs, `UPPER_SNAKE` for constants and macros
- One responsibility per file — separate sensor drivers, network handlers, and control logic
- Header files use include guards: `#ifndef FILENAME_H` / `#define FILENAME_H`
- Keep functions under 40 lines — extract helpers for readability

## Compiler Discipline

- Compile with `-Wall -Wextra -Werror` — no warnings allowed without documented justification
- Use `static` for file-scoped functions and variables — minimize global namespace pollution
- Prefer `const` and `constexpr` over `#define` for typed constants
- Mark unused parameters explicitly: `(void)param;`

## Error Handling

- Every function that can fail must return a status code or use an error parameter
- Never silently swallow errors — log at minimum, recover or escalate at best
- Use enum error codes, not raw integers: `enum class SensorError { OK, TIMEOUT, CRC_FAIL };`
- Critical failures (heap exhaustion, WDT trigger) must log to persistent storage before reset

## Code Organization

```
src/
  main.cpp          — setup() and loop(), minimal logic
  sensors/          — one file per sensor driver
  network/          — WiFi, MQTT, HTTP handlers
  control/          — tank automation logic
  config/           — pin definitions, thresholds, secrets.h
include/
  *.h               — public interfaces only
```

## Static Analysis & Linting

- Run `cppcheck` or PlatformIO's built-in checks before committing
- Address all high/medium severity findings — suppress only with inline justification comment
- Use `clang-format` with project `.clang-format` file for consistent style

## Testability

- Separate hardware access behind interfaces — mock sensors in unit tests
- Pure logic functions (PID calculations, threshold checks) must be testable without hardware
- Use PlatformIO's `test/` framework for native unit tests

## Documentation

- Every public function: one-line `@brief` describing what it does, not how
- Document units in variable names or comments: `temperatureCelsius`, `intervalMs`
- Pin assignments documented in a central `pins.h` with physical location comments
