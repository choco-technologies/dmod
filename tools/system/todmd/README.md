# todmd - DMD Dependencies Generator

## Overview

The `todmd` tool reads a module's required dependencies and generates a `.dmd` file that can be used with `dmf-get` to download all required modules.

## Features

- Loads DMF modules in crossplatform mode (without execution)
- Extracts required module dependencies
- Filters out system modules automatically
- Generates `.dmd` files compatible with `dmf-get`
- Supports modules with or without version specifications

## Usage

```bash
todmd path/to/file.dmf [output.dmd]
```

### Arguments

- `path/to/file.dmf` - Path to the DMF module file (required)
- `[output.dmd]` - Output .dmd file path (optional, defaults to `module_name.dmd`)

### Options

- `-h, --help` - Print help message
- `-v, --version` - Print version information

## Examples

```bash
# Generate dependencies file with default name
todmd myapp.dmf
# Creates: myapp.dmd

# Generate dependencies file with custom name
todmd myapp.dmf dependencies.dmd
# Creates: dependencies.dmd
```

## Generated File Format

The tool generates a `.dmd` file with the following format:

```dmd
# DMOD Dependencies File
# Generated from module: example_app
# Source file: path/to/example_app.dmf
#
# This file lists all non-system modules required by the module.
# Use with dmf-get: dmf-get -d example_app.dmd

dmodex@0.1
other_module@1.2.3
another_module
```

## How It Works

1. Enables crossplatform mode using `Dmod_SetCrossplatformMode(true)` to allow loading modules without execution
2. Loads the specified DMF module using `Dmod_LoadFile()`
3. Iterates through required modules using `Dmod_GetNextRequiredModule()`
4. Filters out system modules by checking the `SystemModule` flag in `Dmod_RequiredModule_t`
5. Writes non-system modules to the output `.dmd` file

## Integration with dmf-get

The generated `.dmd` file can be used directly with the `dmf-get` tool to download all dependencies:

```bash
# Generate dependencies file
todmd myapp.dmf

# Download all dependencies using dmf-get
dmf-get -d myapp.dmd
```

## Notes

- System modules (like `Dmod@0.1`) are automatically filtered out from the generated file
- If a module has no non-system dependencies, an empty `.dmd` file is created with appropriate comments
- The tool requires the module file to be a valid DMF format
- Module loading is done in crossplatform mode, so architecture and version checks are skipped

## See Also

- [dmf-get](../dmf-get/README.md) - Package manager for downloading DMF modules
- [DMD File Format](../../../docs/dmd-file-format.md) - Documentation on .dmd file format
- [todmfc](../todmfc/) - Tool for compressing DMF files
- [todmp](../todmp/) - Tool for creating DMP packages
