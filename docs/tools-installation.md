# DMOD Tools Installation Guide

## Overview

The **DMOD (Dynamic Modules)** framework ships with a set of command-line tools that support building, managing, and distributing dynamic modules. These tools are particularly useful in the build process when using [dmod-boot](https://github.com/choco-technologies/dmod-boot).

## Available Tools

| Tool | Description | Documentation |
|------|-------------|---------------|
| **dmf-get** | DMOD package manager for downloading and managing modules from manifest files | [dmf-get-tool.md](dmf-get-tool.md) |
| **todmm** | Manifest generator - creates a manifest.dmm from a folder of DMF files | [todmm-tool.md](todmm-tool.md) |
| **todmfc** | Tool for compressing DMF files to DMFC format | [todmfc/README.md](../tools/system/todmfc/README.md) |
| **todmp** | DMP package creator - creates packages containing multiple modules | [todmp/README.md](../tools/system/todmp/README.md) |
| **todmd** | DMD dependencies file generator - extracts dependencies from modules | [todmd/README.md](../tools/system/todmd/README.md) |
| **whereisdmf** | Tool for locating module files in configured repository directories | [whereisdmf/README.md](../tools/system/whereisdmf/README.md) |
| **mkdmrpkg** | Release package maker - assembles a release directory from a `.dmr` resource file | [mkdmrpkg-tool.md](mkdmrpkg-tool.md) |

## Environment Variables

DMOD tools use the following environment variables for configuration:

| Variable | Used by | Description | Default Value |
|----------|---------|-------------|---------------|
| **DMOD_TOOLS_NAME** | dmf-get | Tools name for substitution in manifest URLs (e.g., `arch/x86_64`, `arch/armv7/cortex-m7`) | `arch/x86_64` |
| **DMOD_DMF_DIR** | dmf-get | Output directory for DMF files | `./dmf` |
| **DMOD_DMFC_DIR** | dmf-get | Output directory for DMFC files | `./dmfc` |
| **DMOD_MANIFEST** | dmf-get | Default path or URL to manifest file | - |
| **DMOD_REPO_DIR** | All | Module repository directory used during installation | System dependent |

## Building the Project with Tools

### Requirements

Before starting the build, make sure you have installed:

- **GCC Compiler** or compatible
- **CMake** version 3.18 or newer (or Make 4.2+)
- **libcurl** (required for dmf-get)

#### Installing Dependencies on Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev
```

#### Installing Dependencies on Other Linux Systems:

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install gcc gcc-c++ cmake libcurl-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake curl
```

### Building with CMake

1. **Clone the repository:**
```bash
git clone https://github.com/choco-technologies/dmod.git
cd dmod
git submodule update --init --recursive
```

2. **Configure the project in SYSTEM mode with tools enabled:**
```bash
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
```

3. **Build the project:**
```bash
cmake --build build/
```

After successful build, the tools will be located in the `build/bin/tools/` directory.

### Building with Make

1. **Clone the repository:**
```bash
git clone https://github.com/choco-technologies/dmod.git
cd dmod
git submodule update --init --recursive
```

2. **Navigate to the selected tool directory and build:**
```bash
cd tools/system/dmf-get
make
```

Or build all tools from the main directory:
```bash
make -C tools/system/dmf-get
make -C tools/system/todmm
make -C tools/system/todmfc
make -C tools/system/todmp
make -C tools/system/todmd
make -C tools/system/whereisdmf
make -C tools/system/mkdmrpkg
```

## Installing Tools on Linux

### Installing with CMake

After building the project, you can install the tools on your system:

```bash
sudo cmake --install build/ --component tools
```

By default, tools will be installed in `/usr/local/bin`. You can change the installation prefix:

```bash
sudo cmake --install build/ --component tools --prefix /custom/path
```

After installation, tools will be available from anywhere in the system:

```bash
dmf-get --version
todmfc --help
whereisdmf mymodule
```

### Installing with Make

To install an individual tool:

```bash
cd tools/system/dmf-get
sudo make install
```

Installation to a custom directory:

```bash
sudo make install INSTALL_PREFIX=/custom/path
```

### Uninstalling

To uninstall a tool installed with Make:

```bash
cd tools/system/dmf-get
sudo make uninstall
```

For tools installed with CMake, remove files manually:

```bash
sudo rm /usr/local/bin/dmf-get
sudo rm /usr/local/bin/todmm
sudo rm /usr/local/bin/todmfc
sudo rm /usr/local/bin/todmp
sudo rm /usr/local/bin/todmd
sudo rm /usr/local/bin/whereisdmf
sudo rm /usr/local/bin/mkdmrpkg
```

## Tool Details

### dmf-get

**dmf-get** is a package manager for DMOD that enables downloading modules from manifest files (.dmm) and managing dependencies.

**Basic usage:**
```bash
# Download latest version of module
dmf-get mymodule

# Download specific version
dmf-get mymodule@1.0

# Download version matching condition
dmf-get mymodule@>=1.0

# Download all modules from dependencies file
dmf-get -d dependencies.dmd

# Use custom manifest
dmf-get -m http://example.com/manifest.dmm mymodule
```

Full documentation: [dmf-get-tool.md](dmf-get-tool.md)

### todmm

**todmm** scans a folder of DMF/DMFC module files and generates a `.dmm` manifest file, with a configurable base URL prepended to each entry.

**Basic usage:**
```bash
# Generate manifest.dmm from all modules in a folder
todmm ./dmf https://registry.example.com/modules

# Specify a custom output file with -o
todmm ./dmf https://registry.example.com/modules -o custom.dmm
```

**Parameters:**
- `<folder>` - Folder containing `.dmf` / `.dmfc` module files
- `<base_url>` - Base URL prepended to each module file entry
- `-o <file>` - Optional output path (default: `manifest.dmm`)

Full documentation: [todmm-tool.md](todmm-tool.md)

### todmfc

**todmfc** compresses DMF files to DMFC format, reducing their size.

**Basic usage:**
```bash
# Compress with default settings (fastlz, level 2)
todmfc input.dmf output.dmfc

# Specify compression method and level
todmfc input.dmf output.dmfc fastlz 3

# Display available compression methods
todmfc --help
```

**Parameters:**
- `path/to/file.dmf` - Input DMF file
- `path/to/output.dmfc` - Output DMFC file
- `[compression_method]` - Optional compression method (default: fastlz)
- `[level]` - Optional compression level (default: 2)

### todmp

**todmp** creates DMP packages that can contain multiple DMF or DMFC modules and be loaded together.

**Basic usage:**
```bash
# Create package from modules in directory
todmp mypackage ./modules

# Specify output file and main module
todmp kernel ./dmfc main-app ./out/kernel.dmp

# Display contents of DMP package
todmp -l ./mypackage.dmp
```

**Parameters:**
- `<package_name>` - Package name (for the header)
- `<input_dir>` - Directory with modules to package (.dmf or .dmfc)
- `[output_file]` - Optional path to output .dmp file
- `[module_name]` - Optional name of main module in package

### todmd

**todmd** reads module dependencies and generates a .dmd file that can be used with dmf-get to download all required modules.

**Basic usage:**
```bash
# Generate .dmd file with default name
todmd myapp.dmf

# Specify custom output file name
todmd myapp.dmf custom_deps.dmd
```

**Features:**
- Loads DMF modules in cross-platform mode (without execution)
- Extracts required module dependencies
- Automatically filters system modules
- Generates .dmd files compatible with dmf-get

Full documentation: [todmd/README.md](../tools/system/todmd/README.md)

### whereisdmf

**whereisdmf** locates DMOD module files in configured repository directories.

**Basic usage:**
```bash
# Find module for current architecture
whereisdmf mymodule

# Find module for specific architecture
whereisdmf mymodule x86_64
whereisdmf mymodule armv7-cortex-m7
```

**Features:**
- Uses DMOD API (`Dmod_FindModuleFile`) for searching
- Useful in scripts and automation
- Helps debug module location issues
- Verifies module installations

Full documentation: [whereisdmf/README.md](../tools/system/whereisdmf/README.md)

### mkdmrpkg

**mkdmrpkg** assembles a release package directory from a `.dmr` resource file. It
copies files from their `[origin]` locations into an output directory, reproducing
the package structure. The resulting directory can be archived (e.g. with `zip`) to
produce the final release package.

**Basic usage:**
```bash
# Assemble package into ./package
mkdmrpkg module.dmr -m mymodule

# Custom package name (output goes to ./mymodule-1.0.0/)
mkdmrpkg module.dmr -m mymodule --name mymodule-1.0.0

# Append CI-generated files to the package
mkdmrpkg module.dmr -m mymodule -n mymodule-1.0.0 --add-file release-notes.txt
```

**Key options:**
- `<file.dmr>` - Path to the `.dmr` resource file
- `-n`/`--name <name>` - Package name, used as the output directory when `-o` is not set
- `-o <dir>` - Explicit output directory (overrides `--name`)
- `--add-file <path>` - Copy an extra file into the output root (repeatable)
- `-m <module>` - Value for `${module}` variable substitution
- `-r <repo_dir>` - Value for `${repo_dir}` variable substitution
- `-b <build_dir>` - Value for `${build_dir}` variable substitution

Full documentation: [mkdmrpkg-tool.md](mkdmrpkg-tool.md)

## Docker Image

The DMOD framework is available as a ready-to-use Docker image that contains all DMOD tools and a compilation environment for embedded systems.

### Basic Information

- **Image name:** `chocotechnologies/dmod`
- **Current version:** `1.0.4`
- **Base platform:** Ubuntu 20.04
- **Repository:** [Docker Hub](https://hub.docker.com/r/chocotechnologies/dmod)

### Image Contents

The Docker image contains:

- **DMOD Tools:** dmf-get, todmm, todmfc, todmp, todmd, whereisdmf, mkdmrpkg
- **Compilers:**
  - GCC arm-none-eabi (version 10.3-2021.10) — for ARM Cortex-M targets
  - Xtensa ESP toolchain (version 14.2.0_20260121) — for ESP32/ESP32-S3/ESP32-S2 targets (`xtensa-esp32s3-elf-gcc`, `xtensa-esp-elf-gcc`, etc.)
- **Build system:** CMake (version 3.31.3), Make
- **Development tools:** OpenOCD, gcovr, git, jq, zip/unzip
- **Libraries:** libcurl, libusb
- **Configured environment variables:**
  - `DMOD_DMF_DIR=/tools/dmf`
  - `DMOD_DMFC_DIR=/tools/dmfc`
  - `PATH` includes `/usr/local/bin` with DMOD tools, `/tools/gcc-arm-none-eabi/bin`, and `/tools/xtensa-esp-elf/bin`

### Using the Docker Image

**Pull the image:**
```bash
docker pull chocotechnologies/dmod:1.0.4
```

**Run container interactively:**
```bash
docker run -it chocotechnologies/dmod:1.0.4 bash
```

**Use DMOD tools from the container:**
```bash
# Run dmf-get in container
docker run --rm chocotechnologies/dmod:1.0.4 dmf-get --version

# Mount local directory and download modules
docker run --rm -v $(pwd):/workspace -w /workspace \
    chocotechnologies/dmod:1.0.4 dmf-get mymodule
```

**Build project in container (x86_64):**
```bash
# Mount project directory and build
docker run --rm -v $(pwd):/project -w /project \
    chocotechnologies/dmod:1.0.4 bash -c "cmake -B build && cmake --build build"
```

**Build project in container (ESP32-S3 / Xtensa):**
```bash
# Mount project directory and build for ESP32-S3
docker run --rm -v $(pwd):/project -w /project \
    chocotechnologies/dmod:1.0.4 bash -c \
    "mkdir -p build && cd build && cmake .. -DDMOD_TOOLS_NAME=arch/xtensa/esp32s3 && cmake --build ."
```

### Customizing the Image

You can extend the Docker image with additional tools:

```dockerfile
FROM chocotechnologies/dmod:1.0.4

# Add your own tools or configuration
RUN apt-get update && apt-get install -y your-package

# Set custom environment variables
ENV MY_CUSTOM_VAR=value

WORKDIR /workspace
```

## Typical Workflow with Tools

### 1. Environment Setup

```bash
# Option A: Use Docker
docker pull chocotechnologies/dmod:1.0.4
docker run -it -v $(pwd):/workspace -w /workspace chocotechnologies/dmod:1.0.4 bash

# Option B: Install tools locally
git clone https://github.com/choco-technologies/dmod.git
cd dmod
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/
sudo cmake --install build/ --component tools
```

### 2. Downloading Modules

```bash
# Set environment variables
export DMOD_TOOLS_NAME=arch/armv7/cortex-m7
export DMOD_DMF_DIR=./modules/dmf
export DMOD_DMFC_DIR=./modules/dmfc

# Download modules from manifest
dmf-get mymodule@>=1.0
dmf-get -d dependencies.dmd
```

### 3. Module Compression

```bash
# Compress modules to reduce size
todmfc ./modules/dmf/mymodule.dmf ./modules/dmfc/mymodule.dmfc
```

### 4. Package Creation

```bash
# Create DMP package from multiple modules
todmp mypackage ./modules/dmfc ./output/mypackage.dmp main_module
```

### 5. Verification

```bash
# Check module location
whereisdmf mymodule

# Display package contents
todmp -l ./output/mypackage.dmp

# Generate dependencies file from module
todmd mymodule.dmf dependencies.dmd
```

## Troubleshooting

### Error: "libcurl not found"

**Problem:** CMake cannot find the libcurl library during build.

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# Fedora/RHEL
sudo dnf install libcurl-devel
```

### Error: "Module not found" in whereisdmf

**Problem:** whereisdmf cannot find the module.

**Solution:**
1. Check if module exists in `DMOD_DMF_DIR` or `DMOD_DMFC_DIR` directories
2. Ensure environment variables are properly set
3. Verify the architecture matches

### Error: Tools not available after installation

**Problem:** Cannot run tools after installation.

**Solution:**
1. Check if installation directory is in PATH:
```bash
echo $PATH | grep -o "/usr/local/bin"
```

2. If not, add to PATH:
```bash
export PATH=$PATH:/usr/local/bin
# Add to ~/.bashrc or ~/.zshrc to persist
```

## Additional Resources

- **Main repository:** [github.com/choco-technologies/dmod](https://github.com/choco-technologies/dmod)
- **DMOD Boot:** [github.com/choco-technologies/dmod-boot](https://github.com/choco-technologies/dmod-boot)
- **DMF format documentation:** [dmd-file-format.md](dmd-file-format.md)
- **DMM format documentation:** [dmm-file-format.md](dmm-file-format.md)
- **Docker Hub:** [hub.docker.com/r/chocotechnologies/dmod](https://hub.docker.com/r/chocotechnologies/dmod)

## License

DMOD is available under the MIT license. See [license.md](../license.md) for details.
