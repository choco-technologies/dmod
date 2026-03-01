# DMOD Module Generator

This script automates the creation of new DMOD modules from templates, reducing manual configuration and ensuring consistency across modules.

## Overview

The `new-module.sh` script creates a new DMOD module with all necessary files:
- `CMakeLists.txt` - CMake build configuration
- `Makefile` - Make build configuration  
- `<module_name>.c` - Source file with template code
- `README.md` - Module documentation
- `.gitignore` - Git ignore rules
- Optional: GitHub Actions workflow (`.github/workflows/ci.yml`)
- Optional: Bitbucket pipeline (`bitbucket-pipelines.yml`)

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
- `--path PATH` - Path to the folder where the module should be created

### Optional Parameters

- `--author AUTHOR` - Author name (default: "John Doe")
- `--license LICENSE` - License name (default: "MIT")
- `--dmod-dir DIR` - Path to DMOD repository (default: `../../..` for internal modules)
- `--github` - Generate GitHub Actions workflow configuration
- `--bitbucket` - Generate Bitbucket pipeline configuration
- `--dif` - Add DIF (Dynamic Interface) support (library modules only)
- `--mal` - Add MAL (Module Abstraction Layer) support
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

### Create an External Module (Outside DMOD Repository)

```bash
./scripts/new-module.sh \
  --name external_module \
  --type application \
  --path /path/to/external/module \
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

Library modules can optionally implement:
- **DIF (Dynamic Interface)** - Custom interface definitions
- **MAL (Module Abstraction Layer)** - Hardware abstraction interfaces

### Application Module

An application module provides a standalone application with a main entry point. It includes:
- `dmod_preinit()` - Optional pre-initialization function  
- `main()` - Main application entry point

Application modules can optionally implement:
- **MAL (Module Abstraction Layer)** - Hardware abstraction interfaces

## Generated Files

### CMakeLists.txt

The generated CMakeLists.txt includes:
- Module name, version, and author configuration
- Appropriate `dmod_add_library()` or `dmod_add_executable()` call
- Stack size and priority settings (for applications)

For external modules (when `--dmod-dir` is specified with a non-default path), the CMakeLists.txt includes additional setup:
- DMOD_DIR configuration
- External module setup call

### Makefile

The generated Makefile includes:
- Module configuration (name, version, author)
- Source file lists (C and C++ sources)
- Include directories and libraries
- DIF/MAL interface declarations (if specified)
- Reference to DMOD build system

### Source File

The generated source file (`<module_name>.c`) includes:
- Template code with `dmod.h` include
- `dmod_preinit()` function (optional)
- `dmod_init()` function (library) or `main()` function (application)
- `dmod_deinit()` function (library only)
- Documentation comments

### README.md

The generated README includes:
- Module name and description
- Author and license information
- Build instructions for CMake and Make
- Usage instructions

### CI/CD Pipelines

#### GitHub Actions (--github)

Generates `.github/workflows/ci.yml` with:
- CMake build pipeline
- Make build pipeline
- Automatic testing on push/pull requests

#### Bitbucket Pipelines (--bitbucket)

Generates `bitbucket-pipelines.yml` with:
- CMake build step
- Make build step
- Automatic pipeline execution

## Building Generated Modules

### Using CMake

```bash
cd /path/to/module
mkdir -p build
cd build
cmake .. -DDMOD_MODE=DMOD_MODULE
cmake --build .
```

The generated DMF file will be in `build/dmf/<module_name>.dmf` (internal modules) or `build/dmf/<module_name>.dmf` (external modules).

### Using Make

```bash
cd /path/to/module
make DMOD_MODE=DMOD_MODULE
```

The generated DMF file will be in the DMOD repository's `build/dmf/` directory.

## Internal vs External Modules

### Internal Modules

Internal modules are part of the DMOD repository structure. They:
- Use relative paths (`../../..`) for DMOD_DIR
- Are typically in the `examples/` or `modules/` directory
- Share the DMOD build directory

**Example:**
```bash
./scripts/new-module.sh \
  --name my_internal_module \
  --type library \
  --path ./examples/my_internal_module
```

### External Modules

External modules are independent projects that use DMOD as a library. They:
- Specify an absolute path to DMOD_DIR via `--dmod-dir`
- Can be located anywhere in the filesystem
- Have their own build directory
- Include additional setup in CMakeLists.txt

**Example:**
```bash
./scripts/new-module.sh \
  --name my_external_module \
  --type application \
  --path ~/projects/my_module \
  --dmod-dir /path/to/dmod
```

## Notes

- The script validates all input parameters and provides helpful error messages
- Module names should follow C identifier rules (alphanumeric and underscores)
- The script prevents overwriting existing directories
- DIF interfaces are only supported for library modules
- Generated modules include `.gitignore` to exclude build artifacts

## Troubleshooting

### "Directory already exists" Error

The script will not overwrite existing directories. Either:
- Choose a different path
- Remove the existing directory
- Rename the existing directory

### "Template directory not found" Error

Ensure you're running the script from the DMOD repository root or that the script can locate the templates directory.

### Build Errors

If you encounter build errors:
1. Verify DMOD is properly configured (see main DMOD README)
2. Check that `tools-cfg.cmake` or `tools-cfg.mk` exists
3. Ensure DMOD_MODE is set to DMOD_MODULE
4. For external modules, verify the DMOD_DIR path is correct

## See Also

- [DMOD Repository README](../README.md)
- [Module Templates](../templates/module/README.md)
- [Library Module Template](../templates/module/library/README.md)
- [Application Module Template](../templates/module/application/README.md)
