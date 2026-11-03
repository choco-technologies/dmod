# Debian package

DMOD builds a native Debian package named `dmod`, containing:

- `dmod_loader`
- `dmf-get`, `dmf-man`, `whereisdmf`
- `todmfc`, `todmp`, `todmd`, `todmm`, `mkdmrpkg`

Executables are installed in `/usr/bin`, with the license and these instructions
in `/usr/share/doc/dmod`. SDK headers, static libraries and example modules are
not included. Ordinary CMake installation still defaults to `/usr/local`.

The same build also produces `dmod-dev_<version>_all.deb`, containing the
target-neutral module SDK. Install it alongside `dmod` to build modules with
`find_package(dmod CONFIG REQUIRED)` without a source checkout. See
[the development package guide](development-package.md).

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

## GitHub releases

The `Release Debian packages` workflow (`.github/workflows/release.yml`) builds
and tests native packages on Ubuntu 24.04 for amd64 and arm64 (Cortex-A53).
Push a version tag on the commit you want to release, after merging the workflow:

```sh
git tag v1.2.3
git push origin v1.2.3
```

Tags use `vMAJOR.MINOR` or `vMAJOR.MINOR.PATCH`, optionally followed by a
prerelease suffix such as `-rc.1`. The tag supplies the Debian package version;
it does not change DMOD's API or CMake SDK compatibility version. For example,
`v1.2.3` produces:

- `dmod_1.2.3_amd64.deb`
- `dmod_1.2.3_arm64.deb`
- `dmod-dev_1.2.3_all.deb`
- `SHA256SUMS`

Both architectures must pass package metadata, executable, module loading and
installed SDK tests before publication. CI creates a draft GitHub Release,
uploads all assets and then publishes it with generated release notes. Only
the publishing job has `contents: write`; the built-in `GITHUB_TOKEN` is enough.
Uploading before publication also supports
[GitHub immutable releases](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository).

A tag such as `v1.2.3-rc.1` publishes a GitHub prerelease, with Debian version
`1.2.3~rc.1` so APT orders it before `1.2.3`. Published releases are not
overwritten. If publication fails while the release is still a draft, rerun the
failed jobs to upload the assets again and finish publishing.

Pull requests changing the release workflow or packaging configuration, and
manual runs on branches, build and test preview packages with version
`0.0.0~ci.<run-number>`. They only upload Actions artifacts. Manual runs on tags
use the tag version and also only upload artifacts; publication requires a tag
push. Download a runtime package matching your architecture plus the shared SDK,
then verify and install them:

```sh
sha256sum --check --ignore-missing SHA256SUMS
sudo apt-get install ./dmod_1.2.3_amd64.deb ./dmod-dev_1.2.3_all.deb
```

For a local build with an explicit package version, pass
`-DDMOD_DEB_VERSION=1.2.3` when enabling `DMOD_BUILD_DEB`. It defaults to the
project version for ordinary builds.

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
