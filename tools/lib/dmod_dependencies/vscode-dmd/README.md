# DMOD Dependencies Language Support

This extension provides syntax highlighting for DMOD Dependencies (`.dmd`) files.

## Features

- Syntax highlighting for `.dmd` files
- Comment support (`#`)
- Highlighting for:
  - Module entries with optional versions (`module@version`)
  - Version range constraints (`module@>=1.0`, `module@<=2.0`, `module@>=1.0<=2.0`)
  - Configuration file paths (`module config/path.ini`)
  - Custom destination names (`module config/path.ini custom_name.ini`)
  - Include directives (`$include`)
  - Source directives (`$from`)

## Installation

### From Source

1. Copy this directory to your VS Code extensions folder:
   - **Windows**: `%USERPROFILE%\.vscode\extensions`
   - **macOS/Linux**: `~/.vscode/extensions`

2. Restart VS Code

### From VSIX (if packaged)

```bash
code --install-extension dmod-dependencies-1.0.0.vsix
```

## Usage

The extension will automatically activate when you open a `.dmd` file.

## Example

```dmd
# This is a comment

# Simple module without version
dmffs
make_dmffs

# Module with specific version
driver@1.0

# Module with version range constraints
spi@>=1.0         # Version 1.0 or newer
uart@<=2.0        # Version 2.0 or older
i2c@>=1.0<=2.0    # Version between 1.0 and 2.0

# Module with configuration file
dmclk board/stm32f746g-disco.ini
uart@1.2.3 configs/uart_115200.ini

# Module with configuration and custom destination name
gpio configs/gpio_default.cfg custom_gpio.cfg

# Include another dependencies file
$include http://repo.com/other.dmd

# Change the manifest source
$from https://repo.com/manifest.dmm

# Modules after this will use the new manifest
can@>=1.5
```

## Language Support

- **Comments**: Lines starting with `#` are treated as comments
- **Module entries**: Module names with optional `@version` suffix
- **Version ranges**: Support for version constraints with operators:
  - `@>=1.0` - Greater than or equal to 1.0
  - `@<=2.0` - Less than or equal to 2.0
  - `@>1.0` - Greater than 1.0
  - `@<2.0` - Less than 2.0
  - `@>=1.0<=2.0` - Combined range (1.0 to 2.0)
- **Configuration paths**: Module entries can include configuration file paths
  - Format: `module[@version] config_path [custom_dest_name]`
  - Example: `dmclk@1.0 board/stm32f746g-disco.ini`
  - Example with custom name: `gpio config/default.cfg my_gpio.cfg`
- **Include directive**: `$include <url>` to include other `.dmd` files
- **From directive**: `$from <manifest_url>` to change the manifest source

## Related

- [DMOD Manifest Language Support](../dmod_manifest/vscode-dmm) - Syntax highlighting for `.dmm` manifest files
- [DMOD Repository](https://github.com/choco-technologies/dmod) - Dynamic Modules library

## License

MIT License - See repository root for details
