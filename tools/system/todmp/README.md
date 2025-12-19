# todmp - DMP Package Creator

## Overview

`todmp` (to DMP) is a command-line tool for creating DMP (DMOD Module Package) files. DMP packages allow bundling multiple DMF or DMFC modules together into a single file, making it easier to distribute and manage collections of related modules.

## Features

- **Package multiple modules**: Combine multiple DMF/DMFC files into one DMP package
- **Main module designation**: Specify which module is the main entry point
- **Package listing**: View contents of existing DMP packages
- **Automatic module discovery**: Scans directory for all modules
- **Dependencies file support**: Create packages from .dmd files specifying module lists (CMake build only)
- **Simple distribution**: Single file for multiple modules

> **Note**: `.dmd` file support is only available when building with CMake. When building with Make, only directory-based packaging is supported.

## Usage

### Creating a DMP Package from Directory

```bash
todmp <package_name> <input_dir> [output_file] [module_name]
```

**Arguments:**
- `<package_name>` - Name of the package (stored in package header)
- `<input_dir>` - Directory containing .dmf or .dmfc files to package
- `[output_file]` - (Optional) Path to output .dmp file (default: `./package_name.dmp`)
- `[module_name]` - (Optional) Name of the main module in the package

### Creating a DMP Package from .dmd File (CMake build only)

```bash
todmp <package_name> <dmd_file> <input_dir> [output_file] [module_name]
```

**Arguments:**
- `<package_name>` - Name of the package (stored in package header)
- `<dmd_file>` - Path to .dmd file specifying which modules to include
- `<input_dir>` - Directory containing .dmf or .dmfc files
- `[output_file]` - (Optional) Path to output .dmp file (default: `./package_name.dmp`)
- `[module_name]` - (Optional) Name of the main module (defaults to first module in .dmd file)

> **Note**: This feature requires building todmp with CMake. It is not available when building with Make.

### Listing Package Contents

```bash
todmp -l <package_file>
todmp --list <package_file>
```

### Options

```bash
todmp -h, --help        # Display help message
todmp -v, --version     # Display version information
todmp -l, --list <file> # List contents of a DMP package
```

## Examples

### Create a package with default output location

```bash
todmp mypackage ./modules
```

Creates `./mypackage.dmp` containing all modules from `./modules` directory.

### Create a package with custom output path

```bash
todmp mypackage ./modules ./output/mypackage.dmp
```

### Create a package with main module specified

```bash
todmp kernel ./dmfc main-app ./out/kernel.dmp
```

Creates a kernel package where `main-app` is designated as the main module.

### Create a package from .dmd file

**Create a .dmd file** (`dependencies.dmd`):
```dmd
# My project dependencies
core_module
network_stack
filesystem
```

**Create the package:**
```bash
todmp myapp dependencies.dmd ./modules ./myapp.dmp
```

This creates `./myapp.dmp` containing only the three modules listed in `dependencies.dmd`. The first module (`core_module`) will be automatically set as the main module.

### Create a package from .dmd file with custom main module

```bash
todmp myapp dependencies.dmd ./modules ./myapp.dmp network_stack
```

This creates the package with `network_stack` as the main module instead of the first one in the .dmd file.

### List package contents

```bash
todmp -l ./mypackage.dmp
```

Output example:
```
Reading DMP package: ./mypackage.dmp

Package Information:
  Name: mypackage
  Version: 0x0001
  Module Count: 3
  Main Module Index: 0
  Header Size: 128 bytes

Modules:
  [0] main-app
      Offset: 256 bytes
      Size: 45678 bytes
      [MAIN MODULE]
  [1] helper-lib
      Offset: 45934 bytes
      Size: 12345 bytes
  [2] utilities
      Offset: 58279 bytes
      Size: 8901 bytes
```

## Using .dmd Files with todmp

### What are .dmd files?

`.dmd` (DMOD Dependencies) files provide a way to specify which modules should be included in a DMP package. This is particularly useful when:

- You have a directory with many modules but only need a subset
- You want to maintain reproducible builds with explicit module lists
- You're working with pre-downloaded modules from `dmf-get`
- You want to version-control your package composition

### Benefits of .dmd files

1. **Selective packaging**: Include only specific modules instead of all modules in a directory
2. **Reproducible builds**: Explicitly declare which modules are in your package
3. **Automatic main module**: First module in .dmd file becomes the main module by default
4. **Version control friendly**: Track changes to package composition over time
5. **Integration with dmf-get**: Use the same .dmd files for downloading and packaging

### Example workflow with .dmd files

**Step 1: Create a dependencies file** (`myapp.dmd`):
```dmd
# Core application modules
app_main@1.0
network_stack@2.1
config_manager
logger@1.5

# Storage modules
filesystem
database@3.0
```

**Step 2: Download the modules** (optional, if not already downloaded):
```bash
dmf-get -d myapp.dmd -o ./modules
```

**Step 3: Create the DMP package**:
```bash
todmp myapp myapp.dmd ./modules ./dist/myapp.dmp
```

The resulting package will contain exactly the 6 modules listed in `myapp.dmd`, with `app_main` as the main module.

**Note**: When using .dmd files with todmp, the modules should already be downloaded in the input directory. Version specifiers in the .dmd file are ignored during packaging - todmp will use whichever version of each module is present in the input directory.

## DMP Package Format

A DMP package consists of:
1. **Header**: Contains package metadata (name, version, module count, main module index)
2. **Module Entries Table**: Array of module metadata (names, offsets, sizes)
3. **Module Data**: Concatenated module file contents

The format supports:
- Up to 4,294,967,295 modules (uint32_t limit)
- Module names up to 255 characters
- Both DMF and DMFC files
- Main module designation for application entry point

## Use Cases

### Application Distribution

Bundle an application with all its dependencies:
```bash
todmp myapp ./build/modules ./dist/myapp.dmp main
```

### Library Collections

Package related library modules:
```bash
todmp graphics-libs ./libs ./packages/graphics.dmp
```

### Firmware Updates

Create firmware packages with multiple components:
```bash
todmp firmware-v2 ./firmware-modules ./releases/firmware-v2.dmp bootloader
```

## Integration with DMOD

### Loading DMP Packages

DMP packages can be loaded using the DMOD API:

```c
// Load a specific module from a package
Dmod_Context_t* module = Dmod_LoadFromPackage("package.dmp", "module_name");

// Start the loaded module (if it's an application)
if (module != NULL) {
    Dmod_StartModule(module);
}
```

The DMOD system automatically:
- Locates the DMP package file
- Extracts the requested module from the package
- Resolves dependencies between packaged modules
- Loads modules in the correct order

## Building

### Prerequisites
- DMOD library
- Standard C compiler (GCC or compatible)
- Make or CMake

### Build with Make

```bash
cd tools/system/todmp
make
```

> **Note**: When building with Make, `.dmd` file support is not available. Only directory-based packaging is supported. For `.dmd` file support, build with CMake.

### Build with CMake

```bash
cd tools/system/todmp
cmake -B build -S .
cmake --build build
```

> **Note**: CMake build includes `.dmd` file support for selective module packaging.

## Installation

### System-wide installation

```bash
cd tools/system/todmp
sudo make install
```

By default, installs to `/usr/local/bin`. Use `INSTALL_PREFIX` to change:

```bash
sudo make install INSTALL_PREFIX=/custom/path
```

### Uninstalling

```bash
cd tools/system/todmp
sudo make uninstall
```

## Workflow Example

Complete workflow for creating and distributing a package:

```bash
# 1. Build your modules (MODULE mode)
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build-tools -S .
cmake --build build-tools/  # Build tools including todmfc

cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
cmake --build build

# 2. Compress modules (optional, for smaller size)
# Note: If todmfc is available during build, CMake automatically
#       creates compressed .dmfc versions alongside .dmf files
todmfc build/dmf/module1.dmf build/dmfc/module1.dmfc
todmfc build/dmf/module2.dmf build/dmfc/module2.dmfc

# 3. Create DMP package
todmp myapp build/dmfc myapp.dmp module1

# 4. Verify package contents
todmp -l myapp.dmp

# 5. Distribute myapp.dmp to target systems
```

**Automatic Compression**: When building modules with CMake, if `todmfc` is available in your PATH or in the build tools directory, CMake will automatically create compressed `.dmfc` versions of all modules alongside the `.dmf` files. This means you can skip the manual compression step and directly create DMP packages from the `build/dmfc` directory.

## Error Handling

Common errors and solutions:

**"Cannot open file"**
- Verify the package file exists
- Check read permissions

**"Invalid DMP signature"**
- File is not a valid DMP package
- File may be corrupted

**"Cannot read DMP header"**
- File is truncated or corrupted
- Not a DMP file

**"Cannot allocate memory for module entries"**
- System out of memory
- Package header may be corrupted

**"Cannot read module entries"**
- File is corrupted or incomplete
- Package was not created successfully

## Tips

1. **Use DMFC files**: Compress modules before packaging to reduce DMP size
2. **Organize modules**: Keep related modules in a single directory for easy packaging
3. **Test packages**: Always list package contents after creation to verify
4. **Version packages**: Include version information in package names (e.g., `myapp-v1.2.dmp`)
5. **Document main module**: Clearly indicate which module is the application entry point
6. **Use .dmd files**: Maintain .dmd files to explicitly control which modules are packaged
7. **Integrate with dmf-get**: Use the same .dmd file for downloading and packaging modules

## See Also

- [DMD File Format](../../../docs/dmd-file-format.md) - Detailed documentation of .dmd file syntax
- [DMOD Tools Installation Guide](../../../docs/tools-installation.md) - Complete guide for all DMOD tools
- [todmfc](../todmfc/README.md) - DMF compression tool
- [todmd](../todmd/README.md) - Dependencies file generator
- [dmf-get](../dmf-get/README.md) - DMOD package manager

## License

Part of the DMOD framework. See [license.md](../../../license.md) for details.
