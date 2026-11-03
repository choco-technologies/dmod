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
After merging the workflow, open **Releases → Draft a new release** in GitHub,
choose or create a version tag on the commit you want to release, and click
**Publish release**. This starts CI, which attaches the packages directly to
that release, matching the other DMOD repositories. Publishing a prerelease
starts the same workflow. A tag push alone does not start it.

Tags use `vMAJOR.MINOR` or `vMAJOR.MINOR.PATCH`, optionally followed by a
prerelease suffix such as `-rc.1`. CI passes the tag without `v` to CMake as
`DMOD_RELEASE_VERSION` before bootstrapping the tools. This sets the project and
SDK version, generated `dmod_VERSION_MAJOR`, `dmod_VERSION_MINOR` and
`dmod_VERSION_PATCH` fields, `DMOD_VERSION_STRING`, `version.txt`, and tool
`--version` output. The numeric module compatibility field retains its existing
major/minor encoding; patch releases do not change that encoding. For example,
`v1.2.3` produces these downloads under the release's **Assets**:

- `dmod_1.2.3_amd64.deb`
- `dmod_1.2.3_arm64.deb`
- `dmod-dev_1.2.3_all.deb`
- `SHA256SUMS`

Both architectures must pass package metadata, executable, module loading and
installed SDK tests before the assets are uploaded. Only the upload job has
`contents: write`; the built-in `GITHUB_TOKEN` is enough. CI preserves the
release title, notes and prerelease status. A tag such as `v1.2.3-rc.1` uses
Debian version `1.2.3~rc.1` so APT orders it before `1.2.3`.

If an upload fails, rerun the failed jobs. Existing assets with the same names
are replaced, so retrying an upload is supported. To rebuild assets for an
existing release with the latest workflow, open **Actions → Release Debian
packages → Run workflow**, select the branch containing the workflow fix,
and enter the release tag in `tag`. For example, from the command line:

```sh
gh workflow run release.yml --ref develop -f tag=v1.2.3
```

The workflow builds the source from that tag, even when started from another
branch. Pull requests changing the release workflow or packaging configuration,
and manual runs with an empty `tag`, only upload Actions artifacts with preview
version `1.2.3~ci.<run-number>` and tool/header version `1.2.3-ci.<run-number>`.
These previews deliberately exercise nonzero minor/patch fields and prerelease
suffixes; CI checks all packaged tools and an installed SDK consumer against
the expected version.

Download a runtime package matching your architecture plus the shared SDK and
checksums from the release, then verify and install them:

```sh
sha256sum --check --ignore-missing SHA256SUMS
sudo apt-get install ./dmod_1.2.3_amd64.deb ./dmod-dev_1.2.3_all.deb
```

For a local build, pass `-DDMOD_RELEASE_VERSION=1.2.3` to the first configure
command so all binaries, generated headers and SDK metadata receive that
version. It defaults to `1.0`. The Debian version defaults to the same release,
with `-` replaced by `~` before the prerelease suffix. `DMOD_DEB_VERSION` remains
available as an explicit package-only override.

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
