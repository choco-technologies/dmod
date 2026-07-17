# DMOD Module Generator

This script automates the creation of new DMOD modules from templates, reducing manual configuration and ensuring consistency across modules.

## Overview

The `new-module.sh` script creates a new DMOD module with all the files a real
module repo is expected to have:
- `CMakeLists.txt` / `Makefile` - build configuration (CMake fetches `dmod` via `FetchContent` from GitHub by default)
- `src/<module_name>.c` - source file with template code. For library modules,
  this includes a small example interface (an opaque handle with `_create`/
  `_destroy`/`_is_valid`) implemented using dmod's `dmod_<module>_api(...)`/
  `_api_declaration(...)` macros (dmod's Built-in API pattern - see
  `dm_sw_ring` for a real-world equivalent) and `Dmod_Malloc`/`Dmod_Free`
  (dmod's SAL heap functions) - **not** plain C function declarations or
  libc's `malloc`/`free`, which won't link/aren't guaranteed to exist on an
  embedded target. Replace the example with your module's real API.
- `include/<module_name>.h` - public header (library modules only), declaring
  the same example interface
- `docs/README.md`, `docs/api-reference.md` - documentation skeleton
- `tests/CMakeLists.txt`, `tests/<module_name>_test.c` - a `dmod_add_test()` skeleton
- `manifest.dmm` - DMOD manifest entry
- `<module_name>.dmr` - DMOD resource file. With `DMOD_DMR_PATH` set in
  `CMakeLists.txt` (which the generated file always does), `dmod_add_library`/
  `dmod_add_executable` automatically assembles a ready-to-release package in
  `build/packages/<module_name>/` via `todmp`/`mkdmrpkg` - no manual file
  copying needed, see `dmuart.dmr` for the real-world reference
- `README.md` - module documentation, including a "Project Structure" section
- `.gitignore`
- `scripts/sync-claude.sh` - copy of the Claude Code skills sync script (see below)
- Optional: GitHub Actions workflows (`--github`) - both `.github/workflows/ci.yml`
  (build + run the generated unit test via `dmod_loader`; for a `--port` module
  this also builds and verifies the port module for every discovered
  `DMOD_CPU_FAMILY`) and `.github/workflows/release.yml` (builds a release
  package per architecture - or per `DMOD_CPU_FAMILY` for a `--port` module -
  and uploads it to the GitHub release plus a rolling `vlatest`). The release
  workflow doesn't hardcode module names: it discovers every subdirectory
  under `build/packages/` and packages/uploads/lists each one, so it keeps
  working if you add more modules to the repo later
- Optional: Bitbucket pipeline (`bitbucket-pipelines.yml`)
- Optional: a hardware port module (`<module_name>_port`, via `--port`)

## Native Linux Environment Setup

The `setup-linux-env.sh` script configures a native Linux host in the same way as `Docker/Dockerfile.env`.

It installs:
- Required apt packages (including `gcc`, `g++`, `make`, `openocd`, `gcovr`, `git`, `jq`, `zip`, `unzip`)
- Choco scripts from `https://raw.githubusercontent.com/JohnAmadis/choco-scripts/refs/heads/master/install-choco-scripts.sh`
- ARM GNU toolchain `10.3-2021.10` into `/tools/gcc-arm-none-eabi`
- CMake `3.31.3` into `/usr`
- PATH integration via `/etc/profile.d/dmod-tools.sh` and `~/.bashrc`
- DMOD tool output variables `DMOD_DMF_DIR=/tools/dmf` and `DMOD_DMFC_DIR=/tools/dmfc`

Usage:

```bash
./scripts/setup-linux-env.sh
```

Optional flags:
- `--tools-dir PATH`
- `--arm-version VERSION`
- `--cmake-version VERSION`
- `--skip-choco-scripts`
- `--skip-profile-setup`

## Usage

```bash
./scripts/new-module.sh --name MODULE_NAME --type TYPE --path PATH [OPTIONS]
```

### Required Parameters

- `--name NAME` - Name of the module (used for file names and module identification)
- `--type TYPE` - Type of module to create:
  - `library` - A library module that can be used by other modules
  - `application` - An application module with a main entry point
- `--path PATH` - Path to the folder where the module should be created. It
  may already exist - e.g. a repo you just created on GitHub and cloned
  locally, containing only `README.md`/`LICENSE`. Individual files that
  already exist there are left untouched (a warning is printed for each);
  only files that don't exist yet are written. Use `--force` to overwrite.

### Optional Parameters

- `--author AUTHOR` - Author name (default: "John Doe")
- `--license LICENSE` - License name (default: "MIT")
- `--dmod-dir DIR` - Path to a local dmod checkout. When set, the generated
  `CMakeLists.txt` is pinned to it (via `FETCHCONTENT_SOURCE_DIR_DMOD`)
  instead of fetching `dmod`'s `develop` branch from GitHub, and it is also
  used as the source for the Claude Code skills sync. When omitted, the
  generated module fetches `dmod` from GitHub on first configure - this is
  what every real module repo in the ecosystem does.
- `--github` - Generate GitHub Actions workflows: `ci.yml` and `release.yml`
- `--bitbucket` - Generate Bitbucket pipeline configuration
- `--dif` - Add DIF (DMOD Interface) support (library modules only)
- `--mal` - Add MAL (Module Abstraction Layer) support
- `--port` - Add a hardware port module `<module_name>_port` (library modules
  only) - the split-core/port pattern used by `dmuart`/`dmfmc`
- `--port-arch NAME` - Architecture for the generated port skeleton (default:
  `stm32f7`). Requires `--port`.
- `--force` - Overwrite files that already exist at `--path` instead of
  skipping them
- `--skip-claude-sync` - Don't run `scripts/sync-claude.sh` at the end of generation
- `--help` - Show help message

## Examples

### Create a Simple Library Module

```bash
./scripts/new-module.sh \
  --name mylib \
  --type library \
  --path ./modules/mylib
```

### Create an Application Module with Custom Author

```bash
./scripts/new-module.sh \
  --name myapp \
  --type application \
  --path ./modules/myapp \
  --author "Jane Smith"
```

### Create a Library Module with DIF Interface and GitHub Workflow

```bash
./scripts/new-module.sh \
  --name mylib \
  --type library \
  --path ./modules/mylib \
  --dif \
  --github
```

### Create a Driver Module with a Hardware Port (like dmuart/dmfmc)

```bash
./scripts/new-module.sh \
  --name mydriver \
  --type library \
  --path ./modules/mydriver \
  --port \
  --port-arch stm32f7
```

### Point a Generated Module at a Local dmod Checkout

Useful for local development or CI, where you want to build against the
checked-out `dmod` instead of `develop` on GitHub:

```bash
./scripts/new-module.sh \
  --name my_module \
  --type application \
  --path /path/to/my_module \
  --dmod-dir /path/to/dmod \
  --author "Your Name" \
  --github \
  --bitbucket
```

## Module Types

### Library Module

A library module provides functions and APIs that can be used by other modules. It includes:
- `dmod_preinit()` - Optional pre-initialization function
- `dmod_init()` - Initialization function
- `dmod_deinit()` - De-initialization function
- A public header at `include/<module_name>.h`
- A `<module_name>.dmr` resource file (enables release packaging)

Library modules can optionally implement:
- **DIF (DMOD Interface)** - 1:N interface, discovered dynamically at runtime
- **MAL (Module Abstraction Layer)** - 1:1 pluggable interface implementation
- **A hardware port** (`--port`) - split into `<module_name>` (core) and
  `<module_name>_port` (architecture-specific), selected via `DMOD_CPU_FAMILY`

### Application Module

An application module provides a standalone application with a main entry point. It includes:
- `dmod_preinit()` - Optional pre-initialization function
- `main()` - Main application entry point

Application modules can optionally implement:
- **MAL (Module Abstraction Layer)** - pluggable interface implementation

Like library modules, a `.dmr` is generated and `DMOD_DMR_PATH` is set, so
`build/packages/<module_name>/` is assembled automatically on build.

## Claude Code skills sync (`sync-claude.sh`)

`scripts/sync-claude.sh` copies `.claude/skills/` from the `dmod` repository
into the current repository. It is copied into every module generated by
`new-module.sh` and run once, explicitly, at the end of generation (unless
`--skip-claude-sync` is passed) - the result is meant to be reviewed and
committed like any other generated file, not silently regenerated by a build
step.

Run it again any time you want to pick up newer skills:

```bash
./scripts/sync-claude.sh                       # clone dmod@develop
./scripts/sync-claude.sh --dmod-dir /path/to/dmod   # use a local checkout
./scripts/sync-claude.sh --ref v1.2.0           # pin to a tag/branch
```

It only touches `.claude/skills/` - it never overwrites `.claude/settings.json`,
hooks, or skills you added locally under a different name.

**Run this before working with Claude Code in a module repo generated from
this template**, so the `dmod-ecosystem` skill (architecture map, build
system, driver+port pattern) is available.

## Building Generated Modules

### Using CMake

```bash
cd /path/to/module
mkdir -p build
cd build
cmake ..
cmake --build .
```

The generated DMF file will be in `build/dmf/<module_name>.dmf`.

### Using Make

```bash
cd /path/to/module
make DMOD_MODE=DMOD_MODULE
```

## Notes

- The script validates all input parameters and provides helpful error messages
- Module names should follow C identifier rules (alphanumeric and underscores)
- The script prevents overwriting existing directories
- DIF interfaces and hardware ports are only supported for library modules
- Generated modules include `.gitignore` to exclude build artifacts

## Troubleshooting

### "X already exists, skipping" Warning

This is expected when `--path` points at a directory that already had some
files in it (e.g. `README.md`/`LICENSE` from GitHub's repo creation flow) -
those files are intentionally left untouched. If you want the template's
version instead, pass `--force`, or delete/rename the specific file first.

### "Template directory not found" Error

Ensure you're running the script from the DMOD repository root or that the script can locate the `.github/templates` directory.

### Build Errors

If you encounter build errors:
1. Verify network access to GitHub (CMake fetches `dmod` via `FetchContent` by default) or pass `--dmod-dir` to a local checkout
2. Ensure `DMOD_MODE` is set to `DMOD_MODULE` when building with Make
3. For a local `dmod` checkout, verify the `--dmod-dir` path is correct

## See Also

- [DMOD Repository README](../README.md)
- [CMake Functions Reference](../docs/cmake-functions.md)
- [Module Generation Templates](../.github/templates/README.md)
