# DMD File Format - DMOD Dependencies

## Overview

The `.dmd` (DMOD Dependencies) file format provides a way to specify multiple modules that should be downloaded together. This is particularly useful for managing project dependencies, creating reproducible builds, or distributing complete module sets.

## File Format

A `.dmd` file is a plain text file that contains:
- Module names with optional versions
- Comments for documentation
- Include directives to include other `.dmd` files
- Source directives to change the manifest being used

## Syntax

### Comments

Lines starting with `#` are treated as comments and ignored:

```dmd
# This is a comment
# Comments can be used to document your dependencies
```

### Module Entries

Modules are specified by name, optionally followed by a version or version constraint, and optionally a configuration file path:

```dmd
# Module without version (downloads latest available)
dmffs

# Module with specific version
driver@1.0
spi@2.5.1

# Module with configuration file
dmclk@1.0 mcu/stm32f7.ini
uart@2.5 configs/uart_115200.ini

# Module with configuration but no version
gpio configs/gpio_default.cfg

# Module with version range - all versions >= 1.0
dmffs@>=1.0

# Module with version range - versions <= 2.0
uart@<=2.0

# Module with version range - versions between 1.0 and 2.0 (inclusive)
spi@>=1.0<=2.0

# Version constraints with other operators
i2c@>1.0      # Greater than 1.0 (exclusive)
can@<2.0      # Less than 2.0 (exclusive)
```

#### Configuration Files

Modules can optionally specify a configuration file that should be copied during installation. The configuration file path is relative to the module's configuration directory (as defined in the module's `.dmr` file, or the default `config/` directory).

**Format**: `module[@version] [config_path] [custom_dest_name]`

**Basic usage**:
```dmd
# Install dmclk version 1.0 with mcu/stm32f7.ini configuration
# Copies to: <config-dir>/dmclk/stm32f7.ini
dmclk@1.0 mcu/stm32f7.ini

# Install uart with configuration but use latest version
uart configs/uart_default.ini
```

**Custom destination filename**:
```dmd
# Specify custom destination filename (no module subdirectory)
# Copies to: <config-dir>/clk.ini
dmclk@1.0 mcu/stm32f7.ini clk.ini
```

**Multiple configurations from same driver**:
```dmd
# Use driver version 1.0, but copy configs from different versions
dmclk@1.0                          # Install driver v1.0
dmclk@0.1 mcu/stm32f7.ini         # Copy config from v0.1 to dmclk/stm32f7.ini
dmclk@0.2 mcu/high-speed.ini      # Copy config from v0.2 to dmclk/high-speed.ini
```

This allows using a specific driver version while accessing configuration files from multiple versions of the module.

**Variable substitution in configuration paths**:

Configuration paths support variable substitution using `${VARIABLE_NAME}` syntax. Variables can be defined via command-line options or environment variables:

```dmd
# Use variable in config path
dmclk@1.0 boards/${BOARD}/config.ini

# Variable in both config path and destination
uart@2.0 boards/${BOARD}/uart.ini ${BOARD}_uart.ini
```

Command-line usage:
```bash
# Define variables with -D or --define
dmf-get -d project-deps.dmd --config-dir ./config -D BOARD=stm32f7

# Multiple variables
dmf-get -d deps.dmd --config-dir ./config -D BOARD=stm32f7 -D VERSION=v1
```

Variables are substituted in:
- Configuration file paths (source)
- Custom destination filenames

Variable lookup order:
1. User-defined variables (via `-D` option)
2. Environment variables

If a variable is not found, the original `${VAR}` syntax is kept in the path.

To copy the configuration files during installation, use the `--config-dir` option with dmf-get:

```bash
# Install modules and copy configuration files to ./config directory
dmf-get -d project-deps.dmd --config-dir ./config
```

**Configuration file lookup**:
1. The path specified in the module's `.dmr` file with the `config` or `configs` resource key
2. If not found, the default location: `<module_install_dir>/<module_name>/config/<config_path>`

**Destination naming**:
- **Default**: `<config-dir>/<module_name>/<filename>`
- **With custom name**: `<config-dir>/<custom_dest_name>`

#### Version Constraint Syntax

Version constraints support the following operators:
- `@1.0` - Exact version match
- `@>=1.0` - Greater than or equal to version 1.0
- `@<=2.0` - Less than or equal to version 2.0
- `@>1.0` - Greater than version 1.0 (exclusive)
- `@<2.0` - Less than version 2.0 (exclusive)
- `@>=1.0<=2.0` - Combined constraint: version between 1.0 and 2.0 (inclusive)

Versions follow semantic versioning (major.minor.patch) where omitted parts default to 0.

### From Directive

The `$from` directive changes the manifest source for subsequent modules:

```dmd
# These modules use the default manifest
module1
module2@1.0

# Change manifest source
$from https://another-registry.com/manifest.dmm

# These modules will be downloaded from the new manifest
module3@2.0
module4
```

You can use multiple `$from` directives in a single file to download modules from different sources.

#### Inline $from for Single Module

You can also specify a manifest source for a single module using inline `$from`:

```dmd
# This module uses the default manifest
module1@1.0

# This module uses a specific manifest
module2@2.0 $from https://special-registry.com/manifest.dmm

# Back to default manifest for this module
module3@1.5
```

Configuration files can be combined with inline `$from`:

```dmd
# Module with configuration and custom manifest
dmclk@1.0 mcu/stm32f7.ini $from https://hw-registry.com/manifest.dmm

# Configuration with default manifest
uart@2.5 configs/uart.ini
```

### Include Directive

The `$include` directive allows you to include another `.dmd` file:

```dmd
# Include common dependencies
$include https://example.com/common-deps.dmd

# Include from local file
$include ./local-deps.dmd

# Add project-specific modules
my_custom_module@1.0
```

Included files are processed recursively and can contain any valid `.dmd` syntax, including their own `$include` and `$from` directives.

## Complete Example

```dmd
# Project Dependencies File
# Generated: 2024-11-17

# Core system modules from default registry
dmffs
driver@1.0
make_dmffs

# Include shared dependencies
$include https://registry.example.com/common-modules.dmd

# Change to hardware-specific registry
$from https://hw-vendor.com/manifest.dmm

# Hardware drivers with version ranges
spi@>=1.0<=2.0       # SPI driver, any version between 1.0 and 2.0
i2c@>=2.0            # I2C driver, version 2.0 or newer
uart@1.5             # UART driver, exact version 1.5

# Change to third-party registry
$from https://third-party.org/dmod/manifest.dmm

# Third-party libraries with version constraints
json_parser@>=3.0    # JSON parser, version 3.0 or newer
crypto_lib@<=2.0     # Crypto library, version 2.0 or older
```

## Usage with dmf-get

To download all modules specified in a `.dmd` file:

```bash
# Basic usage
dmf-get -d project-deps.dmd

# With custom output directory
dmf-get -d project-deps.dmd -o ./modules

# With architecture specification
dmf-get -d project-deps.dmd -t arch/armv7/cortex-m7

# From URL
dmf-get -d https://example.com/deps.dmd
```

## Best Practices

### 1. Use Comments for Documentation

```dmd
# Hardware Abstraction Layer
# Version: 2.0
# Last updated: 2024-11-17
hal_spi@2.0
hal_i2c@2.0

# Application Layer
# Dependencies on HAL modules above
app_core@1.0
```

### 2. Group Related Modules

```dmd
# === System Modules ===
dmffs
driver@1.0

# === Communication Stack ===
$from https://comms-registry.com/manifest.dmm
tcp_stack@3.0
mqtt_client@2.1

# === Application Modules ===
$from https://app-registry.com/manifest.dmm
my_application@1.5
```

### 3. Version Pin Critical Dependencies

Always specify versions for critical or production dependencies:

```dmd
# Production dependencies - all version pinned
security_module@2.1.0
crypto_lib@1.8.3
network_stack@4.2.1

# Development/test dependencies - can use latest
test_framework
mock_hardware
```

### 4. Use Include for Shared Dependencies

Create reusable dependency sets:

**common-deps.dmd:**
```dmd
# Common dependencies used across all projects
dmffs
logger@1.0
error_handler@2.0
```

**project-deps.dmd:**
```dmd
# Include common dependencies
$include ./common-deps.dmd

# Project-specific dependencies
my_custom_module@1.0
```

### 5. Document Registry Sources

```dmd
# Default registry: https://default-registry.com/manifest.dmm
# Provides: Core system modules

module1
module2@1.0

# Vendor registry: https://vendor.com/manifest.dmm
# Provides: Hardware-specific drivers
$from https://vendor.com/manifest.dmm

vendor_driver@3.0
vendor_hal@2.5
```

## Error Handling

### Missing Modules

If a module specified in the `.dmd` file is not found in the manifest:
- dmf-get will log an error for that specific module
- Continue downloading other modules
- Exit with non-zero status if any module failed

### Download Failures

If a module download fails:
- Error is logged with details
- Other modules continue downloading
- Summary is displayed at the end showing success/failure counts

### Include Failures

If an `$include` directive fails (file not found, network error):
- Parsing stops immediately
- Error message includes the URL/path that failed
- No modules are downloaded

## Advanced Features

### Recursive Includes

Includes can be nested:

**deps.dmd:**
```dmd
$include base-deps.dmd
$include hw-deps.dmd
my_app@1.0
```

**base-deps.dmd:**
```dmd
$include system-deps.dmd
logger@1.0
```

### Dynamic Manifest Selection

Use `$from` to switch between development and production registries:

```dmd
# Use development versions during development
$from https://dev-registry.com/manifest.dmm
module1
module2

# Use production versions for release builds
# (comment out development, uncomment production)
# $from https://prod-registry.com/manifest.dmm
# module1@1.0.0
# module2@2.0.0
```

## VS Code Extension

Install the DMOD Dependencies VS Code extension for:
- Syntax highlighting
- Comment support
- Keyword highlighting for `$include` and `$from`
- Module name and version highlighting
- Inline `$from` directive support

See `tools/lib/dmod_dependencies/vscode-dmd/` for installation instructions.

## Related Documentation

- [dmf-get Tool](dmf-get-tool.md) - Package manager using .dmd files
- [DMM File Format](dmm-file-format.md) - Manifest file format with version directives
- [DMOD Architecture](../README.md) - Overview of DMOD system

## Compatibility

- **Format Version**: 1.0
- **dmf-get Version**: 1.0+
- **DMOD Version**: Compatible with all DMOD versions that support dmf-get

## Future Enhancements

Planned features for future versions:
- Conditional includes (platform-specific)
- Variable substitution in module names
- Dependency conflict resolution
- Lock file generation for reproducible builds
- Checksum verification
