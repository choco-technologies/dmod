# dmf-get - DMOD Package Manager

`dmf-get` is a command-line tool for downloading and managing DMOD packages from manifest files. It serves as a package manager for DMF (DMOD Module Files).

## Features

- **Manifest Parsing**: Read and parse `.dmm` (DMOD Manifest) files
- **Dependencies Files**: Support for `.dmd` (DMOD Dependencies) files to download multiple modules
- **Automatic Dependency Resolution**: Automatically download module dependencies from `.dmd` files or extract from `.dmf` modules
- **Version Management**: Download specific versions of modules
- **Variable Substitution**: Support for `<tools_name>` and `<arch_name>` variables in URLs
- **Include Directives**: Support for `$include` to include other manifests/dependencies
- **Source Switching**: Change manifest sources with `from:` directive in .dmd files
- **Network Downloads**: Download modules from HTTP/HTTPS URLs using libcurl
- **Flexible Configuration**: Configure via command-line arguments or environment variables
- **ZIP Package Support**: Automatic extraction of ZIP packages and detection of `.dmd` dependency files

## Installation

Build the tool as part of the DMOD project:

```bash
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/
cmake --install build/  # Optional: install to system
```

The tool will be available at `build/bin/tools/dmf-get`.

## Usage

### Basic Usage

#### Single Module Download

```bash
# Download latest version of a module
dmf-get mymodule

# Download specific version
dmf-get mymodule@1.0

# Use custom manifest
dmf-get -m path/to/manifest.dmm mymodule

# Use custom output directory
dmf-get -o /path/to/output mymodule

# Specify tools name for variable substitution
dmf-get -t arch/armv7/cortex-m7 mymodule
```

#### Multiple Modules with Dependencies File (.dmd)

```bash
# Download all modules from a dependencies file
dmf-get -d project-deps.dmd

# With custom output directory
dmf-get -d project-deps.dmd -o ./modules

# From a URL
dmf-get -d https://example.com/deps.dmd

# With architecture specification
dmf-get -d deps.dmd -t arch/armv7/cortex-m7
```

### Command-Line Options

- `-d, --dependencies <path>` - Path or URL to dependencies (.dmd) file
- `-m, --manifest <path>` - Path or URL to manifest file
- `-o, --output-dir <path>` - Output directory for downloaded modules
- `-t, --tools-name <name>` - Tools name for variable substitution
- `-a, --arch-name <name>` - Architecture name for variable substitution
- `--type <dmf|dmfc>` - Prefer dmf or dmfc file type
- `--no-dependencies` - Don't download dependencies automatically
- `-h, --help` - Show help message
- `-v, --version` - Show version information

### Environment Variables

`dmf-get` respects the following environment variables:

- `DMOD_TOOLS_NAME` - Default tools name (e.g., `arch/x86_64`)
- `DMOD_DMF_DIR` - Default DMF output directory
- `DMOD_DMFC_DIR` - Default DMFC output directory
- `DMOD_MANIFEST` - Default manifest path or URL

### Examples

```bash
# Download module using default manifest
export DMOD_MANIFEST=https://registry.example.com/manifest.dmm
dmf-get mymodule

# Download with custom tools name
dmf-get -t arch/armv7/cortex-m7 mymodule@1.0

# Download to specific directory
dmf-get -o ./my_modules mymodule
```

## Dependencies File Format (.dmd)

The dependencies file (`.dmd`) allows you to specify multiple modules to download. It supports:

```
# This is a comment
module_name              # Download latest version
module_name@version      # Download specific version
$include url             # Include another .dmd file
$from manifest_url       # Change manifest source for subsequent modules
```

### Example Dependencies File

```dmd
# Project Dependencies

# Core modules from default manifest
dmffs
driver@1.0
make_dmffs

# Include common dependencies
$include https://example.com/common-deps.dmd

# Change manifest source for hardware modules
$from https://hw-vendor.com/manifest.dmm
spi@1.0
i2c@2.0

# Change to another registry
$from https://third-party.org/manifest.dmm
json_parser@3.2
crypto_lib@1.8
```

For detailed documentation on the `.dmd` format, see [DMD File Format Documentation](../../../docs/dmd-file-format.md).

## Automatic Dependency Resolution

`dmf-get` automatically resolves and downloads module dependencies using two mechanisms:

### 1. Dependencies from ZIP Packages (Preferred Method)

When downloading ZIP packages, `dmf-get` looks for a `.dmd` file with the same name as the module:

- **Example**: For `mymodule.dmf`, it searches for `mymodule.dmd` in the ZIP
- If found, the `.dmd` file is extracted and parsed
- All dependencies listed in the `.dmd` file are automatically downloaded
- Dependencies are downloaded recursively (dependencies of dependencies)

**Usage**:
```bash
# Download module with automatic dependency resolution
dmf-get mymodule

# Disable automatic dependency resolution
dmf-get --no-dependencies mymodule
```

### 2. Dependencies from DMF/DMFC Files (Fallback Method)

If no `.dmd` file is found in a ZIP package, or when downloading standalone `.dmf`/`.dmfc` files, `dmf-get` extracts dependencies directly from the module:

- Enables crossplatform mode for safe module inspection
- Uses `Dmod_ReadRequiredModules()` to read module metadata
- Automatically filters out system modules
- Downloads only non-system dependencies recursively

**Example**:
```bash
# When mymodule.dmf has dependencies but no .dmd file
dmf-get mymodule  # Will extract and download dependencies from the .dmf file
```

### Controlling Dependency Resolution

The `--no-dependencies` flag disables automatic dependency resolution:

```bash
# Download only the requested module, no dependencies
dmf-get --no-dependencies mymodule

# Works with .dmd files too
dmf-get --no-dependencies -d project-deps.dmd
```

**Note**: By default, dependencies are downloaded recursively. Each dependency's own dependencies are also resolved and downloaded automatically.

## Manifest File Format (.dmm)

The manifest file (`.dmm`) uses the following format:

```
# This is a comment
module_name@version url
module_name url
$include url_to_another_manifest
```

### Example Manifest

```dmm
# DMF Modules Registry
mymodule@1.0 https://registry.chocotechnologies.com/modules/<tools_name>/mymodule.dmf
mymodule@1.1 https://registry.com/modules.dmf
mymodule https://registry.chocotechnologies.com/packages/<arch_name>/mymodule-latest.zip

# Second module with local path
secondmodule@0.1 /data/modules/dmf/secondmodule.dmfc

# Include another manifest
$include https://registry-dmf.com/<arch_name>/manifest.dmm
```

### Variable Substitution

The manifest supports two variables that are automatically substituted:

- `<tools_name>` - Replaced with the value from `DMOD_TOOLS_NAME` or `-t` option (e.g., `arch/armv7/cortex-m7`)
- `<arch_name>` - Derived from `<tools_name>` by removing `arch/` prefix and replacing `/` with `-` (e.g., `armv7-cortex-m7`)

## Parser Libraries

### Manifest Parser Library

The manifest parsing functionality is implemented as a separate static library (`libdmod_manifest`) that can be used in other projects. See `tools/lib/dmod_manifest/dmod_manifest.h` for the API documentation.

**Features:**
- Parse manifest files from strings, files, or URLs
- Variable substitution
- Recursive include directives
- Best-match version finding
- Pluggable download function (function pointer)

### Dependencies Parser Library

The dependencies parsing functionality is implemented as a separate static library (`libdmod_dependencies`) that can be used in other projects. See `tools/lib/dmod_dependencies/dmod_dependencies.h` for the API documentation.

**Features:**
- Parse .dmd files from strings, files, or URLs
- Support for module entries with optional versions
- Support for `$include` directive (recursive)
- Support for `from:` directive to change manifest sources
- Pluggable download function (function pointer)

## Testing

Unit tests:
```bash
# Manifest parser library tests
./build/tests/tests_dmod_manifest

# Dependencies parser library tests
./build/tests/tests_dmod_dependencies
```

Integration tests for dmf-get (includes .dmd file tests):
```bash
./tests/integration/test_dmf_get.sh build
```

## Future Improvements

- Dependency resolution and automatic download
- Caching of downloaded modules
- Verify checksums/signatures
- Support for more archive formats
- Progress bars for downloads
- Parallel downloads

## Dependencies

- libcurl - for HTTP/HTTPS downloads
- DMOD library - for file operations and utilities

## License

Same as the DMOD project (MIT License).
