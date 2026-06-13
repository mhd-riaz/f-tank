---
description: "Apply when writing, refactoring, or reviewing any C/C++/Arduino firmware. Enforces ESP32 code quality — naming, file structure, compiler hygiene, error handling, static analysis, testability, and documentation."
applyTo: "**/*.{cpp,c,h,hpp,ino}"
---

# Code Quality — ESP32 Firmware

> Goal: every change compiles clean under `-Wall -Wextra -Werror`, fails loudly on error, and keeps logic testable without hardware.

## NEVER (reject the code if violated)

- NEVER leave a failure path silent — every fallible function returns a status or sets an error out-param.
- NEVER ship compiler warnings; `-Wall -Wextra -Werror` must pass with no suppression unless justified inline.
- NEVER put business logic directly in `setup()`/`loop()` — delegate to modules.
- NEVER use a raw `int` for error states — use a typed `enum class`.

## ALWAYS

- ALWAYS name by convention: `camelCase` (vars/functions), `PascalCase` (classes/structs), `UPPER_SNAKE` (constants/macros).
- ALWAYS encode units into names: `temperatureCelsius`, `intervalMs`, `pressureKpa`.
- ALWAYS mark file-scoped functions/vars `static`; prefer `const`/`constexpr` over `#define` for typed constants.
- ALWAYS guard headers: `#ifndef FILE_H` / `#define FILE_H` / `#endif`.
- ALWAYS keep functions ≤ 40 lines and single-responsibility — extract helpers otherwise.
- ALWAYS mark intentionally unused params: `(void)param;`.

## Error Handling Pattern

```cpp
// Typed, explicit, non-silent
enum class SensorError { OK, TIMEOUT, CRC_FAIL };

SensorError readTemperature(float& outCelsius) {
  if (!sensor.ready()) return SensorError::TIMEOUT;   // fail loud, caller decides
  // ...
  return SensorError::OK;
}
```

- Critical failures (heap exhaustion, WDT trigger) MUST log to persistent storage before reset.

## File Organization (one responsibility per file)

```
src/
  main.cpp      — setup()/loop() only, wiring not logic
  sensors/      — one driver per file
  network/      — WiFi, MQTT, HTTP handlers
  control/      — tank automation / PID logic
  config/       — pins.h, thresholds, secrets.h (git-ignored)
include/        — public interfaces only
```

## Testability (required for logic code)

- Hide hardware behind interfaces so sensors can be mocked.
- Keep pure logic (PID, thresholds, conversions) free of hardware calls so it runs under PlatformIO native `test/`.

## Static Analysis (run before commit)

- Run `cppcheck` / PlatformIO check; resolve all high/medium findings (suppress only with an inline reason).
- Format with `clang-format` using the project `.clang-format`.

## Documentation (minimal, useful)

- One-line `@brief` per public function describing _what_, not _how_.
- Centralize pin assignments in `pins.h` with physical-location comments.

## Self-check before finishing

1. Builds clean under `-Wall -Wextra -Werror`?
2. Every failure path returns/propagates a typed error?
3. Functions ≤ 40 lines, single responsibility, correctly named?
4. Logic separated from hardware and unit-testable?
