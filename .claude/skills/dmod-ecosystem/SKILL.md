---
name: dmod-ecosystem
description: Entry-point map of the DMOD (Dynamic Modules) embedded ecosystem architecture — the core loader, the three inter-module communication mechanisms (Built-in API, MAL, DIF), and the shared CMake/Make build system. Load this first when working in any dmod-* repository (dmod, dmod-boot, dmuart, dmtty, dmfmc, dmell, dm_sw_ring, dmclk, dmgpio, dmini, dmheap, dmvfs, dmffs, dmdevfs, dmdrvi, dmhaman, dmosi*, or a new module created from this template) to understand how the repo fits into the larger system before making changes. Points to companion skills (dmod-module-structure, dmod-boot, dmod-hardware-ports, dmod-coding-conventions) for deeper topics.
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

**Naming convention: snake_case everywhere** — module names, file names,
functions, types (`dm_sw_ring`, `dm_sw_ring_create`, `dm_sw_ring_t`,
`dmfmc_configure_sdram`, ...). Don't introduce camelCase or PascalCase names
in module code (the handful of `Dmod_*`/`Pascal_Case` symbols you'll see, like
`Dmod_Printf`, are dmod's own SAL/core API, not a pattern to copy in module code).

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

1. **Built-in / Module API** — a module's own public functions, declared in
   its header with the `dmod_<module>_api(version, ret, _suffix, (args))`
   macro and defined in the `.c` file with the matching
   `dmod_<module>_api_declaration(version, ret, _suffix, (args))` macro (both
   auto-generated per-module into `<module>_defs.h` by the dmod core build
   from `dmod/scripts/api.h.in` — you only follow the naming convention, never
   hand-write the macro itself). **This is not optional boilerplate**: a
   plain C function prototype/definition in a module's public header will
   compile but **fail to link** (or simply never be found), because the
   loader resolves these calls dynamically rather than through normal static
   linkage. Use this as the default way to expose any function another module
   (or this module's own `tests/`) should be able to call — see
   `dm_sw_ring/include/dm_sw_ring.h` + `dm_sw_ring/src/dm_sw_ring.c` for a
   fully worked example, and the `dmod_<module>_port_api(...)` port variant
   described below.
   - A `dmod_<module>_global_api(...)` / `_global_api_declaration(...)`
     variant also exists, declaring the function in the *global* namespace
     instead of the module's own — reserved for exceptional cases (e.g.
     defining a `printf`-style utility meant to be called unqualified from
     anywhere). Default to the plain `_api`/`_api_declaration` pair unless you
     have a specific reason to go global.
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

## Companion skills for deeper topics

This skill is the map, not the territory — load these alongside it as the
task calls for them, rather than assuming everything is covered here:

- **`dmod-module-structure`** — the standard module repo layout
  (`CMakeLists.txt`, `manifest.dmm`, `docs/`, `tests/`), scaffolding a new
  module with `dmod/scripts/new-module.sh`, and when to extract a reusable
  library out of an application module. Load when creating or restructuring
  a module repo.
- **`dmod-boot`** — the firmware entry point (`dmod-boot`): boot sequence,
  module packaging (`modules.dmp`), testing a locally-built `.dmf` on real
  hardware, and the flash/connect/monitor CMake targets. Load when working
  in `dmod-boot` or tracing how firmware boots into a usable system.
- **`dmod-hardware-ports`** — the driver+port split used by hardware
  modules (`dmuart`, `dmfmc`, ...), `DMOD_CPU_FAMILY`, and the
  `dmod_<module>_port_api` macro. Load when writing or modifying a module
  that talks to real hardware/peripherals.
- **`dmod-coding-conventions`** — module C code style: heap over
  fixed-size buffers, never statically link a dependency into a module,
  keep functions short, and the `_create`/`_destroy` opaque-context pattern
  with a magic-field guard. Load when writing or reviewing module
  implementation code.

For a specific module's exact API, config keys, or behavior, read that
repo's `docs/api-reference.md` and `include/<module>.h` directly rather than
guessing from any of these skills — module APIs evolve independently of them.
