---
name: dmod-module-structure
description: Explains the standard DMOD module repo layout (CMakeLists.txt/Makefile skeleton, manifest.dmm, docs/, tests/), how to scaffold a new module with dmod/scripts/new-module.sh, and when to extract a reusable library out of an application module. Load this when creating a new dmod-* module repo, restructuring an existing one, or deciding whether functionality should become its own library module. Companion to the dmod-ecosystem skill.
---

# DMOD Module Structure

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
