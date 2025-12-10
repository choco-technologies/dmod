# DMR File Format - DMOD Resource File

## Overview

The `.dmr` (DMOD Resource File) file format provides a declarative way to specify resource installation mappings for module packages. It defines which files and directories from a zip package should be installed and where they should be placed, with support for environment variable substitution.

## File Format

A `.dmr` file is a plain text file that contains:
- Resource entries mapping source paths to destination paths
- Comments for documentation
- Environment variable substitution using `${VAR}` syntax
- Special variables for dynamic path resolution

## Syntax

### Comments

Lines starting with `#` are treated as comments and ignored:

```dmr
# This is a comment
# Comments can be used to document your resource mappings
```

### Resource Entries

Resource entries follow the format: `key=source_path => destination_path`

```dmr
# Basic format
dmf=./module.dmf => ${DMOD_DMF_DIR}/module.dmf

# Documentation folder
docs=./docs => ${destination}/docs

# Header files
inc=./include => ${destination}/include

# License file
license=./LICENSE => ${destination}/LICENSE
```

#### Resource Entry Components

- **key**: A descriptive identifier for the resource (e.g., `dmf`, `docs`, `inc`, `license`)
- **source_path**: Path to the resource within the zip package (relative to zip root)
- **destination_path**: Target installation path (can include variable substitutions)

### Environment Variable Substitution

The DMR parser supports variable substitution using `${VAR_NAME}` syntax:

#### Special Variables

Three special variables are automatically set by `dmf-get`:

- **`${destination}`**: The installation destination path (from `-o` flag or `DMOD_DMF_DIR`)
- **`${module}`**: The name of the module being installed
- **`${DMOD_DMF_DIR}`**: DMF directory from environment variable

#### Environment Variables

Any environment variable can be referenced:

```dmr
# Use environment variables
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf
custom_path=./data => ${CUSTOM_INSTALL_PATH}/data
config=./config.json => ${HOME}/.config/dmod/${module}/config.json
```

#### Variable Substitution Examples

```dmr
# Install module file with module name in path
dmf=./myapp.dmf => ${DMOD_DMF_DIR}/${module}.dmf

# Install to custom destination with module subdirectory
docs=./documentation => ${destination}/${module}/docs

# Combine multiple variables
headers=./inc => ${destination}/${module}/include
```

### Resource Types

DMR files can specify different types of resources:

#### DMF/DMFC Resources

These are treated specially in `--mini` mode:

```dmr
# Main module file (always installed)
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf

# Compiled module (always installed)
dmfc=./module.dmfc => ${DMOD_DMF_DIR}/${module}.dmfc
```

#### Documentation

```dmr
# Documentation folder
docs=./docs => ${destination}/${module}/docs

# README file
readme=./README.md => ${destination}/${module}/README.md
```

#### Header Files

```dmr
# Include directory
inc=./include => ${destination}/${module}/include

# Specific header
api_header=./api.h => ${destination}/${module}/include/api.h
```

#### Dependencies

```dmr
# Dependencies definition file
dmd=./module.dmd => ${destination}/${module}.dmd
```

#### License and Legal

```dmr
# License file
license=./LICENSE => ${destination}/${module}/LICENSE

# Third-party notices
notices=./NOTICES.txt => ${destination}/${module}/NOTICES.txt
```

#### Custom Resources

```dmr
# Configuration templates
templates=./templates => ${destination}/${module}/templates

# Example code
examples=./examples => ${destination}/${module}/examples

# Test data
test_data=./test => ${destination}/${module}/test
```

## Complete Example

```dmr
# DMOD Resource File for MyModule
# This file specifies where resources should be installed

# Main module file - always installed
dmf=./mymodule.dmf => ${DMOD_DMF_DIR}/${module}.dmf

# Dependencies file
dmd=./mymodule.dmd => ${destination}/${module}.dmd

# Documentation - skipped in --mini mode
docs=./documentation => ${destination}/${module}/docs
readme=./README.md => ${destination}/${module}/README.md

# Header files - skipped in --mini mode
inc=./include => ${destination}/${module}/include

# Examples - skipped in --mini mode
examples=./examples => ${destination}/${module}/examples

# License - skipped in --mini mode
license=./LICENSE => ${destination}/${module}/LICENSE
notices=./THIRD_PARTY_NOTICES.txt => ${destination}/${module}/NOTICES.txt

# Configuration templates - skipped in --mini mode
templates=./config-templates => ${destination}/${module}/templates
```

## Usage with dmf-get

### Automatic Detection

When `dmf-get` extracts a zip package, it automatically looks for a file named `<module_name>.dmr` in the zip root. If found, it processes the resource mappings.

### Installation Modes

#### Full Installation (Default)

All resources defined in the DMR file are installed:

```bash
dmf-get mymodule
```

This installs:
- DMF/DMFC files
- Documentation
- Header files
- Examples
- License files
- All other resources

#### Mini Installation (`--mini` flag)

Only DMF and DMFC resources are installed:

```bash
dmf-get --mini mymodule
```

This installs:
- DMF/DMFC files only
- Skips: docs, headers, examples, licenses, etc.

The `--mini` mode is useful for:
- Production deployments where only the module binary is needed
- Saving disk space
- Faster installation
- Containerized environments

### Example Workflow

1. Create a `mymodule.dmr` file in your module package
2. Package your module as a zip file with the DMR file at the root
3. Publish the zip to your registry
4. Users can install with: `dmf-get mymodule` or `dmf-get --mini mymodule`

## Best Practices

### 1. Always Include DMF/DMFC Resources

```dmr
# Always specify the main module files
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf
```

### 2. Use Generic Paths with Variables

```dmr
# Good - works for any module name
docs=./docs => ${destination}/${module}/docs

# Avoid - hardcoded module name
# docs=./docs => ${destination}/mymodule/docs
```

### 3. Group Related Resources

```dmr
# === Core Module ===
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf
dmd=./module.dmd => ${destination}/${module}.dmd

# === Documentation ===
docs=./docs => ${destination}/${module}/docs
readme=./README.md => ${destination}/${module}/README.md

# === Development Resources ===
inc=./include => ${destination}/${module}/include
examples=./examples => ${destination}/${module}/examples
```

### 4. Document Resource Purpose

```dmr
# Main module binary - required for functionality
dmf=./mymodule.dmf => ${DMOD_DMF_DIR}/${module}.dmf

# API documentation in HTML format
docs=./api-docs => ${destination}/${module}/docs

# C header files for module integration
inc=./include => ${destination}/${module}/include
```

### 5. Consider Mini Mode

Mark essential resources appropriately:

```dmr
# Essential - always installed (dmf/dmfc keys)
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf

# Optional - skipped in mini mode (other keys)
docs=./docs => ${destination}/${module}/docs
examples=./examples => ${destination}/${module}/examples
```

## Security Considerations

### Path Validation

The DMR parser validates all paths to prevent security issues:

- Rejects paths with dangerous characters: `' " ; | & $ \` < > ( ) { } [ ] ! \\ * ?`
- Prevents command injection attacks
- Blocks paths starting with `-` (to avoid shell option injection)

### Safe Resource Keys

Valid resource keys should contain only:
- Alphanumeric characters: `a-z A-Z 0-9`
- Underscores: `_`
- Hyphens: `-`
- Dots: `.`

### Example of Rejected Paths

```dmr
# INVALID - contains shell metacharacters
bad1=./file.txt => /tmp; rm -rf /

# INVALID - contains quotes
bad2=./file.txt => "/tmp/file"

# INVALID - contains pipe
bad3=./file.txt => /tmp/file | cat

# VALID - safe path
good=./file.txt => ${destination}/file.txt
```

## Error Handling

### Missing Source

If a source path doesn't exist in the zip:
- Warning is logged
- Installation continues with other resources
- No error exit

### Variable Not Found

If a variable cannot be resolved:
- Original `${VAR}` syntax is preserved in the path
- Warning may be logged
- Installation continues

### Invalid Syntax

If a line has invalid syntax:
- Error is logged with line number
- Processing stops
- DMR file is rejected

### Path Validation Failure

If a path contains dangerous characters:
- Error is logged
- Resource is skipped
- Installation continues with other resources

## VS Code Extension

Install the DMOD Resource VS Code extension for:
- Syntax highlighting
- Comment support
- Resource entry highlighting (key, source, destination)
- Variable substitution highlighting (`${VAR_NAME}`)

See `tools/lib/dmod_resource/vscode-dmr/` for installation instructions.

## Related Documentation

- [dmf-get Tool](dmf-get-tool.md) - Package manager using .dmr files
- [DMD File Format](dmd-file-format.md) - Dependencies file format
- [DMM File Format](dmm-file-format.md) - Manifest file format
- [DMOD Architecture](../README.md) - Overview of DMOD system

## Compatibility

- **Format Version**: 1.0
- **dmf-get Version**: 1.0+
- **DMOD Version**: Compatible with all DMOD versions that support dmf-get

## Future Enhancements

Planned features for future versions:
- Conditional resource installation (platform-specific)
- Checksum verification for resources
- Compression options
- Resource dependencies (install order)
- Template file processing
- Post-installation hooks
