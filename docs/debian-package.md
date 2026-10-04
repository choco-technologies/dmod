# Debian package

DMOD can build a single native Debian package named `dmod`, containing:

- `dmod_loader`
- `dmf-get`, `dmf-man`, `whereisdmf`
- `todmfc`, `todmp`, `todmd`, `todmm`, `mkdmrpkg`

Executables are installed in `/usr/bin`, with the license and these instructions
in `/usr/share/doc/dmod`. SDK headers, static libraries and example modules are
not included. Ordinary CMake installation still defaults to `/usr/local`.

## Build

Use a native Linux build on Debian or Ubuntu. Install the build dependencies:

```sh
sudo apt-get install build-essential cmake dpkg-dev libcurl4-openssl-dev unzip
```

From the DMOD source directory, first bootstrap the host tools. The loader's
existing build uses `dmf-get` at configure time to download its built-in module
headers and static libraries (`dmlist`, `dmosi`, `dmosi-posix`, `dmosi-proc`), so
this step is needed on a machine without DMOD tools already installed:

```sh
cmake -S . -B build-deb -DDMOD_TOOLS_NAME=arch/x86_64 \
  -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON \
  -DDMOD_BUILD_TESTS=OFF -DDMOD_BUILD_EXAMPLES=OFF -DDMOD_BUILD_TEMPLATES=OFF
cmake --build build-deb --parallel
cmake -S . -B build-deb -DDMOD_BUILD_DEB=ON \
  -DDMF_GET="$PWD/build-deb/bin/tools/dmf-get"
cmake --build build-deb --target package --parallel
```

These commands target x86-64. For a native ARM64 build targeting Cortex-A53
(for example, 64-bit Raspberry Pi OS), replace `arch/x86_64` with
`arch/aarch64/cortex-a53` in the first command and install `gdb-multiarch`,
which that toolchain configuration requires. The selected DMOD toolchain must
match the build machine and the built-in libraries available in the registry.

The second configure needs network access to the DMOD registry. Packaging itself
does not need root privileges. `DMOD_BUILD_DEB` defaults to `OFF` and requires a
standalone native Linux SYSTEM build with tools enabled. The loader is included
even with `DMOD_BUILD_EXAMPLES=OFF`. Cross compilation is not supported by this
packaging configuration; build on the target architecture instead.

The output is `build-deb/dmod_<version>_<architecture>.deb`, for example
`dmod_1.0_amd64.deb` or `dmod_1.0_arm64.deb`. CPack takes the version from DMOD's
project version and detects the Debian architecture. It generates library
dependencies with `dpkg-shlibdeps` and adds `unzip` for archive installation.
Build on the Debian/Ubuntu release where the package will be used, as library
versions and package names can differ across releases.

After building, packaging can also be repeated with:

```sh
(cd build-deb && cpack -G DEB)
```

## Inspect, install and remove

```sh
dpkg-deb --info build-deb/dmod_*.deb
dpkg-deb --contents build-deb/dmod_*.deb
sudo apt-get install ./build-deb/dmod_*.deb
dmod_loader --help
dmf-get --help
sudo apt-get remove dmod
```

APT installs the declared runtime dependencies along with the package. Modules
downloaded separately by `dmf-get` are not owned or removed by the Debian package.
The packaged loader searches `./dmf` and `./dmfc` relative to the working
directory, matching `dmf-get`'s download defaults. Use `DMOD_REPO_PATHS` to add
other module directories. Explicitly customized `DMOD_REPO_DIR` and
`DMOD_REPO_PATHS` CMake settings are preserved when building the package.
