# todmp - DMP Package Creator

## Overview

`todmp` (to DMP) is a command-line tool for creating DMP (DMOD Module Package) files. DMP packages allow bundling multiple DMF or DMFC modules together into a single file, making it easier to distribute and manage collections of related modules.

## Features

- **Package multiple modules**: Combine multiple DMF/DMFC files into one DMP package
- **Main module designation**: Specify which module is the main entry point
- **Package listing**: View contents of existing DMP packages
- **Automatic module discovery**: Scans directory for all modules
- **Simple distribution**: Single file for multiple modules

## Usage

### Creating a DMP Package

```bash
todmp <package_name> <input_dir> [output_file] [module_name]
```

**Arguments:**
- `<package_name>` - Name of the package (stored in package header)
- `<input_dir>` - Directory containing .dmf or .dmfc files to package
- `[output_file]` - (Optional) Path to output .dmp file (default: `./package_name.dmp`)
- `[module_name]` - (Optional) Name of the main module in the package

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

### Build with CMake

```bash
cd tools/system/todmp
cmake -B build -S .
cmake --build build
```

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

## See Also

- [DMOD Tools Installation Guide](../../../docs/tools-installation.md) - Complete guide for all DMOD tools
- [todmfc](../todmfc/README.md) - DMF compression tool
- [todmd](../todmd/README.md) - Dependencies file generator
- [dmf-get](../dmf-get/README.md) - DMOD package manager

## License

Part of the DMOD framework. See [license.md](../../../license.md) for details.
