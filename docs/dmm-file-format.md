# DMM File Format - DMOD Manifest

## Overview

The `.dmm` (DMOD Manifest) file format provides a central registry of available modules with their download URLs and versions. This allows package managers like `dmf-get` to find and download modules from various sources.

## File Format

A `.dmm` file is a plain text file that contains:
- Module entries with versions and download URLs
- Comments for documentation
- Include directives to include other `.dmm` files
- DMOD version directives to specify compatibility requirements

## Syntax

### Comments

Lines starting with `#` are treated as comments and ignored:

```dmm
# This is a comment
# Comments can be used to document your manifest entries
```

### Module Entries

Modules are specified with name, optional version, and download URL:

```dmm
# Module without version
mymodule https://example.com/modules/mymodule.dmf

# Module with specific version
driver@1.0 https://example.com/modules/driver-1.0.dmf
spi@2.5.1 https://example.com/modules/spi-2.5.1.dmfc
```

### DMOD Version Directive

The `$dmod-version` directive specifies the required DMOD version for subsequent entries. This ensures that modules are only installed on compatible DMOD versions:

```dmm
# Set DMOD version requirement to 1.0
$dmod-version 1.0

# These modules require DMOD 1.0 or compatible versions (1.x)
kernel@1.0 https://registry.example.com/modules/kernel.dmf
driver@2.3.1 https://registry.example.com/modules/driver.dmf

# Change DMOD version requirement to 2.0
$dmod-version 2.0

# These modules require DMOD 2.0 or compatible versions (2.x)
newmodule@1.0 https://registry.example.com/modules/newmodule.dmf
```

#### Version Compatibility

Two versions are considered compatible if they have the same major version number:
- DMOD 1.0 is compatible with DMOD 1.2
- DMOD 1.5 is compatible with DMOD 1.9
- DMOD 1.x is **NOT** compatible with DMOD 2.x

The `--skip-dmod-ver-check` flag can be used with `dmf-get` to bypass this check if needed.

### Variable Substitution

The manifest format supports variable substitution for flexible URL patterns:

#### Tools Name Substitution

The `<tools_name>` variable is replaced with the tools name (e.g., "arch/armv7/cortex-m7"):

```dmm
driver@2.3.1 https://registry.example.com/modules/<tools_name>/driver.dmf
```

#### Architecture Name Substitution

The `<arch_name>` variable is replaced with the architecture name (e.g., "armv7-cortex-m7"):

```dmm
app@1.5 https://registry.com/<arch_name>/app.dmfc
```

#### CPU Name Substitution

The `<cpu_name>` variable is replaced with the specific CPU name (e.g., "stm32f746ngh6"). This allows defining modules for specific microcontrollers:

```dmm
uart@1.0 https://registry.com/<cpu_name>/uart.dmf
```

#### CPU Family Substitution

The `<cpu_family>` variable is replaced with the CPU family name (e.g., "stm32f7"). This enables different module binaries depending on the processor family:

```dmm
# Different UART implementation for stm32f7 vs stm32f4
uart@1.0 https://registry.com/<cpu_family>/uart.dmf
```

#### Version Substitution

The `<version>` variable is replaced with the module version at download time:

```dmm
dmffs https://github.com/example/dmffs/releases/download/v<version>/dmffs-v<version>-<arch_name>.zip
```

#### Multiple Substitutions

You can combine multiple variables in a single URL:

```dmm
mymodule https://cdn.example.com/<tools_name>/<version>/mymodule-<arch_name>.dmf

# CPU-specific module with all variables
uart https://registry.com/<cpu_family>/<cpu_name>/<version>/uart-<arch_name>.dmf
```

### Version Available Directive

The `$version-available` directive specifies which versions are available for a module. When a module entry uses the `<version>` placeholder without an explicit version, it will be automatically expanded into multiple entries using the available versions:

```dmm
# Declare available versions for a module
$version-available dmffs 1.0.0 1.1.0 1.2.0 2.0.0

# This entry will be expanded into 4 entries (one for each version)
dmffs https://github.com/example/dmffs/releases/download/v<version>/dmffs-v<version>-<arch_name>.zip
```

The above is equivalent to manually writing:

```dmm
dmffs@1.0.0 https://github.com/example/dmffs/releases/download/v1.0.0/dmffs-v1.0.0-<arch_name>.zip
dmffs@1.1.0 https://github.com/example/dmffs/releases/download/v1.1.0/dmffs-v1.1.0-<arch_name>.zip
dmffs@1.2.0 https://github.com/example/dmffs/releases/download/v1.2.0/dmffs-v1.2.0-<arch_name>.zip
dmffs@2.0.0 https://github.com/example/dmffs/releases/download/v2.0.0/dmffs-v2.0.0-<arch_name>.zip
```

#### Version Available Rules

- Each `$version-available` directive must specify a module name followed by one or more version strings
- The directive only affects module entries **without an explicit version** (e.g., `mymodule` not `mymodule@1.0`)
- The module entry URL must contain the `<version>` placeholder to be expanded
- Multiple modules can have different available versions by using separate `$version-available` directives
- If a new `$version-available` directive is declared for the same module, it replaces the previous list

#### Example with Multiple Modules

```dmm
# Define available versions for different modules
$version-available kernel 1.0 1.1 1.2
$version-available driver 2.0 2.1

# These entries will be expanded
kernel https://registry.com/kernel-<version>-<arch_name>.dmf
driver https://registry.com/driver-<version>-<arch_name>.dmf

# This entry has explicit version, so it won't be expanded
driver@3.0 https://registry.com/driver-3.0-special-<arch_name>.dmf
```

This creates 3 kernel entries (1.0, 1.1, 1.2), 2 driver entries (2.0, 2.1), plus 1 explicit driver@3.0 entry.

### Include Directive

The `$include` directive allows you to include another `.dmm` file:

```dmm
# Include additional manifest
$include https://registry-dmf.com/<arch_name>/manifest.dmm

# Include with tools_name
$include https://registry.example.com/modules/<tools_name>/additional.dmm
```

Included files are processed recursively and can contain any valid `.dmm` syntax.

## Complete Example

```dmm
# DMOD Manifest Example
# Registry: https://registry.example.com

# Set DMOD version requirement
$dmod-version 1.0

# Core system modules with specific versions
kernel@1.0 https://registry.example.com/modules/kernel.dmf
driver@2.3.1 https://registry.example.com/modules/<tools_name>/driver.dmf

# Define available versions for modules
$version-available dmffs 1.0.0 1.1.0 1.2.0
$version-available mymodule 2.0 2.1 2.2

# Modules with version placeholders - will be expanded using $version-available
dmffs https://github.com/example/dmffs/releases/download/v<version>/dmffs-v<version>-<arch_name>.zip
mymodule https://cdn.example.com/<tools_name>/<version>/mymodule-<arch_name>.dmf

# Local file path
localmodule@0.1 /data/modules/dmf/localmodule.dmfc

# Include another manifest
$include https://registry-dmf.com/<arch_name>/manifest.dmm

# Module without version (no expansion since no <version> in URL)
testmodule https://example.com/test.dmf

# Change DMOD version requirement for newer modules
$dmod-version 2.0

# These require DMOD 2.x
newfeature@1.0 https://registry.example.com/modules/newfeature.dmf
```

## Usage with dmf-get

To use a manifest with `dmf-get`:

```bash
# Use default manifest
dmf-get mymodule@1.0

# Specify custom manifest
dmf-get -m https://example.com/manifest.dmm mymodule

# Skip DMOD version check
dmf-get --skip-dmod-ver-check mymodule

# Specify CPU name and family for CPU-specific modules
dmf-get --cpu-name stm32f746ngh6 --cpu-family stm32f7 uart

# Combine architecture and CPU options
dmf-get -a armv7-cortex-m7 --cpu-name stm32f746ngh6 --cpu-family stm32f7 uart@1.0
```

## Best Practices

### 1. Document DMOD Version Requirements

```dmm
# === DMOD 1.x Compatible Modules ===
$dmod-version 1.0

kernel@1.0 https://example.com/kernel.dmf
driver@2.0 https://example.com/driver.dmf

# === DMOD 2.x Compatible Modules ===
$dmod-version 2.0

newmodule@1.0 https://example.com/newmodule.dmf
```

### 2. Group Related Modules

```dmm
# === Core System Modules ===
kernel@1.0 https://registry.com/kernel.dmf
driver@2.3.1 https://registry.com/driver.dmf

# === Communication Stack ===
tcp_stack@3.0 https://comms.com/tcp.dmf
mqtt_client@2.1 https://comms.com/mqtt.dmf
```

### 3. Use Version Placeholders with Version Available

```dmm
# Define available versions
$version-available dmffs 1.0.0 1.1.0 1.2.0

# Entry is automatically expanded for all available versions
dmffs https://github.com/example/dmffs/releases/download/v<version>/dmffs-<version>-<arch_name>.zip
```

### 4. Provide Multiple Architecture Support

```dmm
# Works with any architecture via <arch_name>
driver@1.0 https://registry.com/<arch_name>/driver.dmf
```

## Error Handling

### Module Not Found

If a module is not found in the manifest:
- Error message is logged
- Download fails unless `--ignore-missing` flag is used

### DMOD Version Mismatch

If a module requires incompatible DMOD version:
- Warning is logged
- Module is skipped
- Next compatible entry is tried
- Can be bypassed with `--skip-dmod-ver-check`

### Download Failures

If a module download fails:
- Error is logged with details
- Process continues with other modules (if applicable)
- Exit code reflects failure

## Related Documentation

- [DMD File Format](dmd-file-format.md) - Dependencies file format
- [dmf-get Tool](dmf-get-tool.md) - Package manager documentation
- [DMOD Architecture](../README.md) - Overview of DMOD system

## Compatibility

- **Format Version**: 1.0
- **dmf-get Version**: 1.0+
- **DMOD Version**: 1.0+

## Future Enhancements

Planned features for future versions:
- Module metadata (author, description, license)
- Checksum/signature verification
- Mirror support for fallback URLs
- Conditional entries based on platform
