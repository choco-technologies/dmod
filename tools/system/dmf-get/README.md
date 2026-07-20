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

# Same as above, using 'install' subcommand (compatible with other package managers)
dmf-get install mymodule

# Download specific version
dmf-get mymodule@1.0

# Use custom manifest
dmf-get -m path/to/manifest.dmm mymodule

# Use custom output directory
dmf-get -o /path/to/output mymodule

# Specify tools name for variable substitution
dmf-get -t arch/armv7/cortex-m7 mymodule

# Download module and copy configuration file
dmf-get mymodule@1.0 --config board/config.ini --config-dir ./config

# Example: Download module with specific config (as requested in issue)
dmf-get dmclk@0.4 --config board/stm32f746g-disco.ini --config-dir ./config

# Download with custom config destination name
dmf-get mymodule --config mcu/default.ini --config-dir ./cfg --config-dest my.ini
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

#### Extracting Specific Resources

`dmf-get` supports extracting only specific resources from module packages:

```bash
# Extract only header files
dmf-get headers mymodule
dmf-get headers mymodule@1.0 -o /path/to/output

# Extract only documentation
dmf-get docs mymodule
dmf-get docs mymodule@1.0 -o /path/to/output

# Extract only the static library
dmf-get lib mymodule
dmf-get lib mymodule@1.0 -o /path/to/output
```

The `lib` command extracts a module's precompiled static library (the `lib` resource from its `.dmr` file, e.g. `libmymodule.a`) to `$DMOD_LIB_DIR` or `$DMOD_DMF_DIR/lib/<module>/lib` by default.

```bash
# Extract and use the static library
dmf-get lib mymodule
ls $DMOD_DMF_DIR/lib/mymodule/lib/libmymodule.a
```

The `docs` command extracts module documentation to `$DMOD_DOC_DIR` or `$DMOD_DMF_DIR/<module>/docs` by default. After extraction, you can view the documentation using the `dmf-man` tool:

```bash
# Extract and view documentation
dmf-get docs mymodule
dmf-man mymodule

# Or use a custom documentation directory
dmf-get docs mymodule -o /path/to/docs
dmf-man -d /path/to/docs mymodule
```

For more information about viewing documentation, see the [dmf-man tool documentation](../dmf-man/README.md).

#### Copying Configuration Files from Modules

When downloading a single module from the command line, you can specify a configuration file to copy from the module package using the `--config` option. This feature is similar to the configuration file support in `.dmd` files.

```bash
# Copy a configuration file from the module to a destination directory
dmf-get dmclk@0.4 --config board/stm32f746g-disco.ini --config-dir ./config

# This will:
# 1. Download and install the dmclk@0.4 module
# 2. Look for board/stm32f746g-disco.ini in the module's config directory
# 3. Copy it to ./config/dmclk/stm32f746g-disco.ini

# Specify a custom destination filename (without module subdirectory)
dmf-get mymodule --config mcu/default.ini --config-dir ./cfg --config-dest my.ini
# Copies to: ./cfg/my.ini (instead of ./cfg/mymodule/default.ini)

# Use with variable substitution
dmf-get mymodule --config boards/${BOARD}/config.ini --config-dir ./config -D BOARD=stm32f7
# Substitutes ${BOARD} with stm32f7 in the config path
```

**Configuration File Lookup:**
1. The configuration file is searched in the module's config directory as specified in the `.dmr` file
2. If not found in `.dmr`, the default location `<output-dir>/<module-name>/config/<config-path>` is used

**Destination Naming:**
- **Default behavior**: Configuration file is copied to `<config-dir>/<module-name>/<filename>`
- **With --config-dest**: Configuration file is copied to `<config-dir>/<custom-name>`

**Requirements:**
- Both `--config` and `--config-dir` must be specified together
- The module must be successfully installed before the configuration file is copied
- If configuration file copying fails, the module installation still succeeds (with a warning)

#### Routing Tagged Configuration Files with `--config-map`

When downloading modules from a `.dmd` dependencies file, each configuration entry can be tagged by prefixing the config path with `<tag>=`. Combined with `--config-map`, this routes configuration files to different destination directories based on their tag, instead of all of them going to the single `--config-dir`.

```dmd
# project-deps.dmd
mymodule driver=board/stm32f746g-disco.ini
anothermodule service=generic/service.ini
```

```bash
dmf-get -d project-deps.dmd --config-map "driver:./config/drivers;service:./config/services"
```

This copies `mymodule`'s configuration to `./config/drivers/mymodule/stm32f746g-disco.ini` and `anothermodule`'s configuration to `./config/services/anothermodule/service.ini`.

**`--config-map` format:** `<tag>:<dir>;<tag2>:<dir2>;...`

**Notes:**
- `--config-map` requires `-d`/`--dependencies` (it has no effect on single-module `--config` downloads)
- Tags are optional per `.dmd` entry; untagged entries fall back to `--config-dir`
- If an entry's tag has no matching `--config-map` mapping, `dmf-get` falls back to `--config-dir` (with a warning) if one was given, otherwise the configuration file is skipped
- A custom destination filename can still be combined with a tag: `driver=mcu/stm32f7.ini clk.ini`

### Command-Line Options

- `-d, --dependencies <path>` - Path or URL to dependencies (.dmd) file
- `-m, --manifest <path>` - Path or URL to manifest file
- `-o, --output-dir <path>` - Output directory for downloaded modules
- `--config <path>` - Configuration file to copy from module (for single module only)
- `--config-dir <path>` - Directory where configuration files should be copied
- `--config-dest <name>` - Custom destination filename for configuration file
- `--config-map <map>` - Route tagged configuration files (`<tag>=path` in `.dmd`) to different directories, e.g. `"driver:./config/drivers;service:./config/services"` (requires `-d`)
- `-D, --define <VAR=value>` - Define variable for configuration path substitution
- `-t, --tools-name <name>` - Tools name for variable substitution
- `-a, --arch-name <name>` - Architecture name for variable substitution
- `--type <dmf|dmfc>` - Prefer dmf or dmfc file type
- `--no-dependencies` - Don't download dependencies automatically
- `--no-fallback` - Don't fall back to the public manifest if the module is not found in the provided manifest
- `-y, --yes` - Automatic yes to license prompts (non-interactive mode)
- `-c, --clear-cache` - Clear downloaded manifests and packages cache
- `-h, --help` - Show help message
- `-v, --version` - Show version information

## Caching

`dmf-get` caches downloaded manifests and package files to reduce network requests and speed up subsequent downloads. The cache is stored in `~/.cache/dmod/dmf-get/` by default.

### Cache Structure

- `~/.cache/dmod/dmf-get/manifests/` - Cached manifest files
- `~/.cache/dmod/dmf-get/packages/` - Cached package files (DMF/DMFC and ZIP files)

### Clearing the Cache

To clear all cached data:

```bash
dmf-get --clear-cache
```

Or use the short form:

```bash
dmf-get -c
```

### Custom Cache Directory

You can customize the cache location using the `DMOD_CACHE_DIR` environment variable:

```bash
export DMOD_CACHE_DIR=/path/to/custom/cache
dmf-get mymodule
```

### Environment Variables

`dmf-get` respects the following environment variables:

- `DMOD_TOOLS_NAME` - Default tools name (e.g., `arch/x86_64`)
- `DMOD_DMF_DIR` - Default DMF output directory
- `DMOD_DMFC_DIR` - Default DMFC output directory
- `DMOD_MANIFEST` - Default manifest path or URL
- `DMOD_CACHE_DIR` - Cache directory (default: `~/.cache/dmod/dmf-get`)

### Examples

```bash
# Download module using default manifest
export DMOD_MANIFEST=https://registry.example.com/manifest.dmm
dmf-get mymodule

# Download with custom tools name
dmf-get -t arch/armv7/cortex-m7 mymodule@1.0

# Download to specific directory
dmf-get -o ./my_modules mymodule

# Download module with configuration file (as requested in issue)
dmf-get dmclk@0.4 --config board/stm32f746g-disco.ini --config-dir ./config

# Non-interactive mode (automatically accept licenses)
dmf-get -y mymodule
```

## License Acceptance

When downloading modules that include a license file (specified in the `.dmr` resource file), `dmf-get` will display the license and prompt for acceptance before installation:

```
================================================================================
LICENSE
================================================================================
MIT License

Copyright (c) 2024 Module Author
...
================================================================================

Do you accept the license terms? [y/N]:
```

The user must enter `y` or `Y` to accept the license and continue with installation. Any other input (including just pressing Enter) will reject the license and cancel the installation.

### Non-Interactive Mode

For automated installations or CI/CD pipelines, use the `-y` or `--yes` flag to automatically accept all licenses:

```bash
# Automatically accept licenses without prompting
dmf-get -y mymodule

# Can be combined with other options
dmf-get -y -d dependencies.dmd -o ./output
```

**Note**: Modules without license files will install normally without any prompts.

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

- ~~Caching of downloaded modules~~ ✓ Implemented
- Verify checksums/signatures
- Support for more archive formats
- Progress bars for downloads
- Parallel downloads

## Dependencies

- libcurl - for HTTP/HTTPS downloads
- DMOD library - for file operations and utilities

## License

Same as the DMOD project (MIT License).
