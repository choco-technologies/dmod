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
todmd path/to/file.dmf [output.dmd] [-r version_requirements.txt] [--from manifest_or_mapping ...]
```

### Arguments

- `path/to/file.dmf` - Path to the DMF module file (required)
- `[output.dmd]` - Output .dmd file path (optional, defaults to `module_name.dmd`)

### Options

- `-h, --help` - Print help message
- `-v, --version` - Print version information
- `-r <file>` - Version requirements file from `dmod_link_modules` (optional)
- `--from <manifest>` - Global manifest file/URL; adds a `$from` directive at the top of the generated `.dmd` so all modules are fetched from that manifest (optional)
- `--from <module>=<manifest>` - Per-module manifest mapping; adds an inline `$from <manifest>` only for the named module (optional, repeatable)

## Examples

```bash
# Generate dependencies file with default name
todmd myapp.dmf
# Creates: myapp.dmd

# Generate dependencies file with custom name
todmd myapp.dmf dependencies.dmd
# Creates: dependencies.dmd

# Generate dependencies file with version requirements
todmd myapp.dmf -r version_requirements.txt
# Creates: myapp.dmd with versions from version_requirements.txt

# Use a global manifest for all modules
todmd myapp.dmf --from build/manifest.dmm
# Creates: myapp.dmd with "$from build/manifest.dmm" at the top

# Use a global manifest URL for all modules
todmd myapp.dmf --from https://registry.example.com/manifest.dmm
# Creates: myapp.dmd with "$from https://registry.example.com/manifest.dmm" at the top

# Use different manifests for specific modules
todmd myapp.dmf --from dmgpio=build/manifest.dmm --from dmclk=build/manifest2.dmm
# Creates: myapp.dmd with inline "$from" directives for dmgpio and dmclk
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

dmodex@>=0.1<1.0
other_module@>=1.2.3<2.0
another_module
```

When a global manifest is provided via `--from`:

```dmd
# DMOD Dependencies File
# ...

$from build/manifest.dmm

dmodex@>=0.1<1.0
other_module@>=1.2.3<2.0
another_module
```

When per-module manifests are provided via `--from module=manifest`:

```dmd
# DMOD Dependencies File
# ...

dmodex@>=0.1<1.0 $from build/manifest.dmm
other_module@>=1.2.3<2.0
another_module $from build/manifest2.dmm
```

> **Note:** When a dependency version is auto-discovered from the DMF (i.e., not explicitly specified
> by the user via `dmod_link_modules`), a soft range constraint (`>=version<(major+1).0`) is generated
> instead of a hard exact constraint. The lower bound allows any newer patch/minor version; the upper
> bound (`<(major+1).0`) ensures only the same major-version series is accepted, since dmod rejects
> cross-major-version API connections.
> If the user explicitly specifies a version in `dmod_link_modules` (e.g., `dmodex@0.1`), the exact
> version is preserved as-is in the generated `.dmd` file.

## How It Works

1. Enables crossplatform mode using `Dmod_SetCrossplatformMode(true)` to allow loading modules without execution
2. Loads the specified DMF module using `Dmod_LoadFile()`
3. Optionally loads version requirements from a file (if `-r` flag is provided)
4. Iterates through required modules using `Dmod_GetNextRequiredModule()`
5. For each module, checks if there's a version requirement specified in the version requirements file
6. Merges version information: version requirements from the file take precedence over versions in the DMF
7. Filters out system modules by checking the `SystemModule` flag in `Dmod_RequiredModule_t`
8. Writes non-system modules with their versions to the output `.dmd` file

## Version Requirements File Format

The version requirements file is a simple text format:

```
# Comment lines start with #
module_name@version
another_module@1.2.3
third_module
```

- Lines starting with `#` are comments
- Each line contains a module name, optionally followed by `@version`
- Empty lines are ignored

## Integration with dmod_link_modules

When using the `dmod_link_modules()` CMake function to specify module dependencies with versions, a version requirements file is automatically created. This file is then automatically passed to `todmd` during the build process, ensuring that:

1. Version constraints specified in your `CMakeLists.txt` are preserved in the generated `.dmd` file
2. These version constraints take precedence over any versions found in the DMF itself
3. The `.dmd` file accurately reflects your build-time dependency requirements

Example workflow:
```cmake
# In CMakeLists.txt
dmod_link_modules(my_module
    dmini@1.0
    dmodex@0.2
)
```

When the module is built, `todmd` automatically receives the version requirements and generates a `.dmd` file that includes these version constraints.

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
