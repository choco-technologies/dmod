# Building modules with dmod-dev

The `dmod-dev` Debian package supplies headers, CMake functions, linker scripts,
templates and module-side runtime sources. It depends on the matching `dmod`
host-tools package, CMake and the native C/C++ build tools. Install both locally
built packages with:

```sh
sudo apt-get install ./build-deb/dmod_*.deb ./build-deb/dmod-dev_*.deb
```

Both packages are produced by the existing `DMOD_BUILD_DEB=ON` configuration and
`cmake --build build-deb --target package`; see [the package build guide](debian-package.md).
The SDK package has Debian architecture `all`: its runtime is compiled from
source using the consuming project's compiler. No DMOD checkout or network
access is needed to build a self-contained module.

## Minimal project

```cmake
cmake_minimum_required(VERSION 3.18)
find_package(dmod 1.0 CONFIG REQUIRED)
project(my_module LANGUAGES C CXX)

dmod_add_library(my_module "0.1" src/my_module.c)
```

```sh
cmake -S . -B build
cmake --build build --parallel
```

The normal DMOD functions are available, including `dmod_add_executable`,
`dmod_add_test` and `dmod_link_modules`. Outputs default to `build/dmf` (`.dmf`
and `.dmd`) and `build/dmfc` (`.dmfc`). `dmod-config.h`, generated API headers
and compiled runtime objects are created in the consumer's build directory;
the installed SDK remains read-only. Existing module API declaration rules
still apply; see the [CMake function reference](cmake-functions.md).

Set DMOD options before `find_package`, for example `DMOD_USE_FASTLZ`,
`DMOD_DMF_DIR`, `DMOD_DMFC_DIR`, `DMOD_REPO_DIR` and `DMOD_ARCH`. Set per-module
options such as `DMOD_AUTHOR_NAME`, `DMOD_STACK_SIZE`, `DMOD_MAL_IMPLS` and
`DMOD_DMR_PATH` before calling `dmod_add_library` or `dmod_add_executable`.
`dmod_link_modules` uses the installed `dmf-get` to obtain dependency headers;
registry downloads still require network access unless the resources are local
or already cached.

To migrate an existing CMake module, replace the DMOD `FetchContent` block,
`include(.../paths.cmake)` and `dmod_setup_external_module()` with
`find_package(dmod CONFIG REQUIRED)` before `project()`. Retain the module's
sources, options, API declarations and calls to the module-building functions.
The installed SDK supports MODULE mode; building the SYSTEM runtime/firmware
or using the legacy Make integration still requires the source checkout.

## Cross compilation and custom prefixes

All existing `DMOD_TOOLS_NAME` profiles are installed with the SDK. They load
the **same** `configs/<name>/tools-cfg.cmake` as source/FetchContent builds,
selecting the compiler, binutils, architecture, CPU/FPU flags and DMOD feature
settings, and selecting the dependency registry configuration. For example:

```sh
cmake -S . -B build-arm -DDMOD_TOOLS_NAME=arch/armv7/cortex-m7
cmake --build build-arm --parallel
```

Place `find_package` before `project()`, just as the existing flow places
`FetchContent_MakeAvailable(dmod)` before `project()`. CMake must choose the
compiler before enabling languages. Selecting a profile afterwards reports an
error instead of silently building with the host compiler. Repeated
`find_package` calls after initialization are supported. Use a fresh build
directory when changing profiles.

The existing `DMOD_TOOLS` override can select a custom tools configuration.
Profile-specific overrides such as `FPU_FLAGS`, `CROSS_COMPILE` and
`CPUCONFIG_CFLAGS` retain their original profile semantics. Install the chosen
compiler separately; `build-essential` only supplies the native compiler, and
ARM profiles also look for the target debugger or `gdb-multiarch`.

Alternatively, omit `DMOD_TOOLS_NAME`/`DMOD_TOOLS` and select your own compiler
with `CMAKE_TOOLCHAIN_FILE`. In that case `find_package` may also follow
`project()`. Set CPU/FPU flags in the toolchain and `DMOD_ARCH`/`DMOD_CPU` if
needed to match the firmware's module architecture.

When a toolchain searches only a target sysroot for packages, set
`dmod_DIR=/usr/share/cmake/dmod` explicitly to select this target-neutral SDK.
Compression and dependency tools continue to run on the host. Cross-built
modules must be tested on their target, not with the host `dmod_loader`.

For a source installation under a different prefix, use `cmake --install`
with `--prefix`, then pass that prefix through `CMAKE_PREFIX_PATH`. SDK paths
are resolved relative to the installed CMake configuration, so the prefix can
also be relocated without embedding a source-checkout path.

## Assets and existing projects

The SDK includes the current shared `dmod_setup_assets` implementation:
`DMOD_ASSETS_PATHS` converts views with `todmv`, images with `todmvi` and the
appropriate decoder, and fonts with `todmvf`, including font `tracking` and
multi-section INI files. The older `DMOD_FIXTURES_PATHS` name still works with
its existing deprecation warning. `.dmr` packaging receives `DMOD_VIEWS_DIR`.
Asset tools run on the **host**, selected by `DMOD_VIEWS_TOOLS_NAME` (default
`arch/x86_64`), independently of the target's `DMOD_TOOLS_NAME`. The SDK locates
the host `dmf-get` and `dmod_loader` outside cross-compilation sysroots.

Using the SDK is optional. Existing projects using a checkout, FetchContent,
`paths.cmake`/`dmod_setup_external_module`, or Make continue to use their
existing flow without any source changes. Installing the packages does not
redirect or replace those flows.
