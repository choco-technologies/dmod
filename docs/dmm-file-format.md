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

#### Version Substitution

The `<version>` variable is replaced with the module version at download time:

```dmm
dmffs https://github.com/example/dmffs/releases/download/v<version>/dmffs-v<version>-<arch_name>.zip
```

#### Multiple Substitutions

You can combine multiple variables in a single URL:

```dmm
mymodule https://cdn.example.com/<tools_name>/<version>/mymodule-<arch_name>.dmf
```

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

# Core system modules
kernel@1.0 https://registry.example.com/modules/kernel.dmf
driver@2.3.1 https://registry.example.com/modules/<tools_name>/driver.dmf

# Modules with version placeholders
dmffs https://github.com/example/dmffs/releases/download/v<version>/dmffs-v<version>-<arch_name>.zip
mymodule https://cdn.example.com/<tools_name>/<version>/mymodule-<arch_name>.dmf

# Local file path
localmodule@0.1 /data/modules/dmf/localmodule.dmfc

# Include another manifest
$include https://registry-dmf.com/<arch_name>/manifest.dmm

# Module without version
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

### 3. Use Version Placeholders for Dynamic URLs

```dmm
# Supports any version via <version> placeholder
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
