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

## `dmod-boot`: the entry point

`dmod-boot` is the bootloader/firmware entry point for a real board. It
initializes hardware, FreeRTOS (via `dmosi-freertos`), heap (`dmheap`), VFS
(`dmvfs`, mounting `dmramfs` on `/`, `dmdevfs` on `/dev`), logging (`dmlog`),
then loads a package of modules embedded directly in ROM at link time and
starts a main module — typically the shell, `dmell`. If you're tracing "how
does firmware boot into a usable system", `dmod-boot/src/main.c` is the
starting point.

**Module packaging.** During the build, `dmod-boot` bundles every `.dmf` it
finds in `<dmod-boot>/build/dmf/` into a single `modules.dmp` package, which
is then embedded in flash alongside the rest of the firmware (`modules/modules.dmd`
is the manifest that determines which modules get fetched/built into
`build/dmf/` in the first place).

**Testing a locally-built module on real hardware, without a release:** drop
the `.dmf` binary straight into `<dmod-boot>/build/dmf/`, then delete
`<dmod-boot>/build/modules.dmp` and `<dmod-boot>/build/__modules_dmp.o` so the
next build is forced to regenerate the package (an incremental build won't
notice a `.dmf` was added/changed in place otherwise).

**Flashing and debugging** (from `<dmod-boot>/build`):
- `cmake --build . --target install-firmware` — flash the firmware to the board.
- `cmake --build . --target connect` — attach OpenOCD to the target.
- `cmake --build . --target monitor` — attach a console to `dmell` over the
  existing OpenOCD connection (run `connect` first). This gives you stdin/stdout
  independent of any firmware-level driver — you get a working shell even
  before a UART driver is loaded, since it doesn't go through `dmuart`/`dmtty`
  at all. Log output for this relies on `dmlog`, the base/kernel-level logging
  system used from very early boot (before most modules are up), which is why
  `dmod-boot` depends on it directly rather than through a loaded module.

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

## Extracting a library out of an application module

Sometimes an `application`-type module accumulates functionality that other
modules need to call — not just as an implementation detail, but as an API
other modules should link against. In that case, split the reusable part
out into its own `library`-type DMOD module rather than exposing the
application module's internals directly, and give that library module a
`lib` prefix. E.g. if the application module is `systemd`, the extracted
library becomes `libsystemd`: `systemd` itself becomes a consumer of
`libsystemd` (via `dmod_link_modules`) alongside every other module that
needs that API, rather than a special case other modules reach into.

## Strings: prefer the heap over fixed-size buffers

The target is embedded (STM32F4/F7, ESP32-S3) — RAM is scarce and every byte
of it matters, so default to heap-allocating rather than reserving fixed
static/stack space "just in case". This applies to strings in general, not
just paths: don't declare a fixed-size `char buf[N]` for a string whose
length isn't tightly bounded — heap-allocate it instead. Beyond the general
RAM-budget argument, fixed-size stack/static buffers silently truncate or
overflow once a value gets longer than whoever wrote `N` expected, which is a
real risk for anything composed at runtime (paths, module names, mount
points under `/`, `.dmr`/`.dmm` entries, log/error messages, etc.). A static
fixed-size array is only acceptable when you can genuinely guarantee the
string will never exceed ~20 bytes (e.g. a short fixed enum-like tag) —
paths and other composed/runtime strings do not qualify.

When you need an owned copy of a string (not just a borrowed pointer), copy
it with `Dmod_StrDup` rather than `malloc` + `strcpy`/`memcpy`, and `free` it
when done.

The same reasoning applies to lists: don't hand-roll a fixed-size static
array for a collection whose size grows/shrinks at runtime. Use the `dmlist`
module instead — pull it in with `dmod_link_modules(${DMOD_MODULE_NAME}
dmlist)` in `CMakeLists.txt` (see `dmtty/CMakeLists.txt` for a working
example) rather than reimplementing a dynamic list.

## Never statically link libraries into a module

Always link a module against another module's `_if` target dynamically
(`dmod_link_modules`, the standard Built-in API / MAL / DIF mechanisms) —
never statically link a library's object code directly into a module. DMOD
modules are loaded/unloaded independently at runtime; statically linking a
dependency into a module defeats that (bloats the `.dmf`, duplicates the
dependency's RAM/flash footprint across every module that links it, and
breaks the loader's ability to resolve/share a single instance of it). If
you find yourself reaching for `target_link_libraries` with anything other
than a module's own `_if` interface target, stop and use
`dmod_link_modules` instead.

## Keep functions short and extract shared logic

Functions should stay short — **max ~50 lines**. If a function is growing
past that, it's a signal to split it, not to keep going. Always look for the
common part between two similar code paths (two callers doing almost the
same thing, two branches with overlapping logic, ...) and pull it into its
own function rather than duplicating it inline.

## Prefer the `_create`/`_destroy` context pattern

The preferred way to manage a piece of stateful data is a context object:
a `_create` function allocates and initializes a context struct holding all
the data that logic needs, callers thread that context pointer through the
rest of the API, and a `_destroy` function frees it once it's no longer
needed. See `dm_sw_ring_create()`/`dm_sw_ring_destroy()` in
`dm_sw_ring/include/dm_sw_ring.h` for a worked example of this shape.

**Context structs must be opaque/private.** Declare only the typedef in the
public/interface header, and define the actual struct body in the private
source file:

```c
/* public header (e.g. include/<module>.h) */
typedef struct context context_t;
```

```c
/* private file (e.g. src/<module>.c or a private header) */
struct context
{
    ...
};
```

This keeps callers from reaching into the struct directly or depending on
its layout — all access must go through the module's API functions.

**Guard context structs with a magic field.** Give the struct a `magic`
field (`uint32_t`) set in `_create` to a fixed value derived from a short
form of the module's name (e.g. a 4-char tag packed into a `uint32_t`), and
check it at the top of every function that receives the context pointer,
before the context is dereferenced (or used) — the check should be present
in every entry point (`_destroy` included), not just a subset. This catches
use of a stale/freed, corrupted, or wrong-type pointer early instead of
leading to undefined behavior further down.

## When you need more detail

This skill gives the map, not the territory. For a specific module's exact
API, config keys, or behavior, read that repo's `docs/api-reference.md` and
`include/<module>.h` directly rather than guessing from this summary — module
APIs evolve independently of this skill.
