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
project(my_module LANGUAGES C CXX)

find_package(dmod 1.0 CONFIG REQUIRED)
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
`find_package(dmod CONFIG REQUIRED)` after `project()`. Retain the module's
sources, options, API declarations and calls to the module-building functions.
The installed SDK supports MODULE mode; building the SYSTEM runtime/firmware
or using the legacy Make integration still requires the source checkout.

## Cross compilation and custom prefixes

Select your target compiler with CMake's `CMAKE_TOOLCHAIN_FILE` **before**
`project()` (normally via the configure command). Install that compiler
separately; `build-essential` only supplies the host compiler. Set the target's
CPU/FPU flags in the toolchain and, if needed, `DMOD_ARCH`/`DMOD_CPU` to match
the firmware's module architecture. The SDK does not select a board for you.

```sh
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake
cmake --build build-arm --parallel
```

When a toolchain searches only a target sysroot for packages, set
`dmod_DIR=/usr/share/cmake/dmod` explicitly to select this target-neutral SDK.
Compression and dependency tools continue to run on the host. Cross-built
modules must be tested on their target, not with the host `dmod_loader`.

For a source installation under a different prefix, use `cmake --install`
with `--prefix`, then pass that prefix through `CMAKE_PREFIX_PATH`. SDK paths
are resolved relative to the installed CMake configuration, so the prefix can
also be relocated without embedding a source-checkout path.
