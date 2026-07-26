---
name: dmod-hardware-ports
description: Explains the driver+port pattern used by DMOD hardware modules (dmuart, dmfmc, dmgpio, dmclk, and any new peripheral driver) — splitting a module into a hardware-independent core and an architecture-specific "_port" module selected via DMOD_CPU_FAMILY, the dmod_<module>_port_api macro, and sharing logic across MCU families via src/port/<family>_common/. Load this when writing or modifying a module that talks to real hardware/peripherals. Companion to the dmod-ecosystem skill.
---

# Driver + port pattern (hardware modules)

Modules that touch real hardware (`dmuart`, `dmfmc`, and any new peripheral
driver) split into **two independent DMOD modules in one repo**:

- `<module>` — the core module: public API, config parsing (via `dmini`),
  hardware-independent logic. Links against `<module>_port_if`.
- `<module>_port` — a second module, built from `src/port/CMakeLists.txt`,
  containing the architecture-specific implementation. One subdirectory per
  MCU family under `src/port/<arch>/` (e.g. `src/port/stm32f7/`), each with a
  `config.cmake` that sets `DMOD_TOOLS_NAME` (e.g.
  `"arch/armv7/cortex-m7"`) and a `port.c` implementing the lifecycle
  (`dmod_init`/`dmod_deinit`) plus any `DMOD_IRQ_HANDLER(...)`.

The active architecture is selected via **`DMOD_CPU_FAMILY`** (a cache
variable already defined in `dmod/dmod-cfg.cmake`, consumed by `dmf-get
--cpu-family` for package resolution) — set it directly in the module's
`CMakeLists.txt` before `FetchContent_MakeAvailable(dmod)`:

```cmake
set(DMOD_CPU_FAMILY "stm32f7" CACHE STRING "Target CPU family")
include(${CMAKE_CURRENT_SOURCE_DIR}/src/port/${DMOD_CPU_FAMILY}/config.cmake)
```

Do **not** invent a module-specific variable like `<MODULE>_MCU_SERIES` for
this — `DMOD_CPU_FAMILY` already exists and is understood by the rest of the
toolchain (`dmf-get`, package naming). `dmfmc`'s `DMFMC_MCU_SERIES` predates
this and is legacy, not a pattern to copy in new modules.

The port's public API is declared with the `dmod_<module>_port_api(version,
return_type, _suffix, (args))` macro — this macro is generated automatically
by the dmod core build from `dmod/scripts/api.h.in` into
`<module>_port_defs.h`; you only need to follow the naming convention, not
hand-write the macro. See `dmfmc/include/dmfmc_port.h` for a full example, or
`dmuart/include/dmuart_port.h` for a simpler one.

If a peripheral IP block is identical across MCU families, keep the shared
logic in `src/port/<family>_common/` and make each `src/port/<arch>/port.c` a
thin wrapper (lifecycle + IRQ only) — see `dmfmc/docs/port-implementation.md`
for the exact recipe used there.
