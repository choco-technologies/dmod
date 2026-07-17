---
name: dmod-ecosystem
description: Explains the DMOD (Dynamic Modules) embedded ecosystem architecture — the core loader, the three inter-module communication mechanisms (Built-in API, MAL, DIF), the shared CMake/Make build system, the standard module repo layout, and the driver+port pattern used by hardware modules. Load this when working in any dmod-* repository (dmod, dmod-boot, dmuart, dmtty, dmfmc, dmell, dm_sw_ring, dmclk, dmgpio, dmini, dmheap, dmvfs, dmffs, dmdevfs, dmdrvi, dmhaman, dmosi*, or a new module created from this template) to understand how the repo fits into the larger system before making changes.
---

# DMOD Ecosystem

DMOD ("Dynamic Modules") is a framework for dynamically loading and running code
on embedded microcontrollers (STM32F4/F7, ESP32-S3 seen so far), similar in spirit
to shared libraries on a desktop OS but built for firmware. A "module" is a
self-contained unit of code (library or application) compiled to a `.dmf`
(DMOD File) — optionally compressed to `.dmfc` — that a loader can load, run,
and unload at runtime without recompiling or resetting the whole firmware.

This ecosystem is spread across many small, independent git repositories under
one parent directory (each repo = one module, or the core framework). There is
no monorepo and no shared parent git tree — treat each repo you open as
self-contained, but expect it to `FetchContent` the `dmod` repo at configure time.

## The core repo: `dmod`

`dmod` is the SDK and build system every other repo depends on. It provides:

- **The loader/runtime** (`src/system/`, `src/module/`) — loads `.dmf`/`.dmfc`/`.dmp`
  files, manages module contexts, resolves inter-module calls.
- **Two build modes**, selected via `DMOD_MODE`:
  - `DMOD_SYSTEM` — builds the loader itself, embedded in firmware.
  - `DMOD_MODULE` — builds a single loadable module (`.dmf`).
- **CMake functions** every module repo uses (see `dmod/docs/cmake-functions.md`
  for the full reference):
  - `dmod_add_library(name version sources...)` / `dmod_add_executable(...)` —
    create a module. Reads `DMOD_MODULE_NAME`, `DMOD_MODULE_VERSION`,
    `DMOD_AUTHOR_NAME`, `DMOD_STACK_SIZE`, optionally `DMOD_PRIORITY`,
    `DMOD_MAL_IMPLS`, `DMOD_DIF_IMPLS`, `DMOD_DMR_PATH` (set before calling).
    Also creates a `<name>_if` INTERFACE target for consumers that only need headers.
  - `dmod_link_modules(target [PRIVATE|PUBLIC|INTERFACE] module[@version]...)` —
    downloads another module's headers via `dmf-get` at configure time and adds
    them as include dirs. This is how modules depend on each other without
    vendoring source.
  - `dmod_setup_external_module()` — call after `project()` in a standalone repo
    to wire up DMOD's CMake machinery.
- **The package manager**: `dmf-get` (installs/downloads modules), plus
  `todmfc`/`todmd`/`todmm`/`todmp`/`mkdmrpkg`/`dmf-man` tools and three manifest
  file formats, all documented under `dmod/docs/`:
  - `.dmm` (manifest) — registry mapping module name → download URL.
  - `.dmr` (resource file) — declares what files go where when a module is
    installed/packaged (`docs/dmr-file-format.md`).
  - `.dmd` (dependencies) — generated list of a module's dependency versions.

## Three ways modules talk to each other

1. **Built-in / Module API** — direct call to another module's exported
   function by name (`<module><function>`), via a generated
   `<module>_defs.h`. Simplest, requires knowing the target module at compile time.
2. **MAL (Module Abstraction Layer)** — 1:1 inversion of control. One module
   defines an interface; another module implements it and registers via
   `DMOD_MAL_IMPLS` in its `CMakeLists.txt`/`Makefile`. Lets you swap an
   implementation (e.g. which UART driver backs a generic "serial" interface)
   without touching the caller.
3. **DIF (Dmod Interface)** — 1:N. Multiple modules implement the same
   interface (e.g. several SPI/UART drivers) and are discovered dynamically at
   runtime via `Dmod_GetNextDifModule()`/`Dmod_GetDifFunction()`, registered
   through `DMOD_DIF_IMPLS`.

Pick Built-in API for a hard compile-time dependency, MAL for "one pluggable
backend", DIF for "many interchangeable backends discovered at runtime".

## `dmod-boot`: the entry point

`dmod-boot` is the bootloader/firmware entry point for a real board. It
initializes hardware, FreeRTOS (via `dmosi-freertos`), heap (`dmheap`), VFS
(`dmvfs`, mounting `dmramfs` on `/`, `dmdevfs` on `/dev`), logging (`dmlog`),
then loads a package of modules embedded directly in ROM at link time
(`modules.dmp`, built from `modules/modules.dmd`) and starts a main module —
typically the shell, `dmell`. If you're tracing "how does firmware boot into a
usable system", `dmod-boot/src/main.c` is the starting point.

## Standard module repo layout

Every real module repo (`dmuart`, `dmtty`, `dmfmc`, `dm_sw_ring`, `dmell`, ...)
follows the same shape:

```
<module>/
├── CMakeLists.txt        # FetchContent's dmod, then dmod_add_library/_executable
├── Makefile               # Make-based equivalent build
├── manifest.dmm           # $include .../versions.dmm + this module's download URL
├── <module>.dmr           # install/package resource mapping
├── include/<module>.h     # public API
├── src/<module>.c         # implementation + dmod_init()/dmod_deinit()
├── docs/
│   ├── README.md           # doc index
│   ├── api-reference.md    # full API reference
│   └── (examples.md, configuration.md, port-implementation.md as needed)
├── tests/
│   ├── CMakeLists.txt       # dmod_add_test(...)
│   └── <module>_test.c      # #include "dmod_test.h", DMOD_TEST_STEP(...) macros
└── README.md               # includes a "Project Structure" tree section
```

Every `CMakeLists.txt` in a standalone module repo follows the same skeleton —
`FetchContent_Declare(dmod GIT_REPOSITORY .../dmod.git GIT_TAG develop)` +
`FetchContent_MakeAvailable(dmod)` + `include(${DMOD_DIR}/paths.cmake)` +
`dmod_setup_external_module()` — rather than assuming a local checkout path.
Look at `dm_sw_ring/CMakeLists.txt` or `dmtty/CMakeLists.txt` for a clean example.

New modules should be scaffolded with `dmod/scripts/new-module.sh` (see
`--help` for `--type library|application`, `--port`, `--dif`, `--mal`), which
generates this exact layout.

## Driver + port pattern (hardware modules)

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

## When you need more detail

This skill gives the map, not the territory. For a specific module's exact
API, config keys, or behavior, read that repo's `docs/api-reference.md` and
`include/<module>.h` directly rather than guessing from this summary — module
APIs evolve independently of this skill.
