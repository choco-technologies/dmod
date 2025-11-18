# DMOD Manifest Language Support

Syntax highlighting for DMOD Manifest (`.dmm`) files in Visual Studio Code.

## Features

This extension provides syntax highlighting for DMOD manifest files, including:

- **Comments**: Lines starting with `#`
- **DMOD version directive**: `$dmod-version` for specifying required DMOD version
- **Version available directive**: `$version-available` for declaring available module versions
- **Module entries**: `module@version url` syntax
- **Include directives**: `$include url` for manifest inclusion
- **Variable placeholders**: `<tools_name>`, `<arch_name>`, `<version>`
- **URLs**: HTTP/HTTPS and file:// URLs

## Manifest Format

DMOD manifest files (`.dmm`) define package modules with the following syntax:

```dmm
# This is a comment

# Set DMOD version requirement
$dmod-version 1.0

# Module with version
mymodule@1.0 https://registry.example.com/modules/mymodule.dmf

# Declare available versions for automatic expansion
$version-available dmffs 1.0.0 1.1.0 1.2.0

# Module without version (will be expanded using available versions)
dmffs https://github.com/example/releases/download/v<version>/dmffs-<arch_name>.zip

# Include another manifest
$include https://registry.example.com/additional-manifest.dmm

# Change DMOD version requirement
$dmod-version 2.0

# Local file reference
localmodule@2.0 /data/modules/localmodule.dmfc
```

### DMOD Version Directive

The `$dmod-version` directive specifies the required DMOD version for subsequent manifest entries:

```dmm
$dmod-version 1.0
```

Entries after this directive require DMOD version 1.x (major version compatibility). Entries with incompatible major versions are automatically filtered during package installation.

### Version Available Directive

The `$version-available` directive declares which versions are available for a module. When a module entry without an explicit version uses the `<version>` placeholder, it is automatically expanded into multiple entries:

```dmm
$version-available dmffs 1.0.0 1.1.0 1.2.0
dmffs https://github.com/example/releases/download/v<version>/dmffs-<version>.zip
```

This automatically expands to:
- `dmffs@1.0.0` with URL `https://github.com/example/releases/download/v1.0.0/dmffs-1.0.0.zip`
- `dmffs@1.1.0` with URL `https://github.com/example/releases/download/v1.1.0/dmffs-1.1.0.zip`
- `dmffs@1.2.0` with URL `https://github.com/example/releases/download/v1.2.0/dmffs-1.2.0.zip`

Each module can have its own list of available versions. Module entries with explicit versions (e.g., `mymodule@2.0`) are not expanded.

### Variable Placeholders

- `<tools_name>`: Replaced with the tools name (e.g., `arch/armv7/cortex-m7`)
- `<arch_name>`: Replaced with architecture name (e.g., `armv7-cortex-m7`)
- `<version>`: Replaced with the requested module version

## Installation

### From Source

1. Copy the `vscode-dmm` directory to your VS Code extensions folder:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\`
   - **macOS/Linux**: `~/.vscode/extensions/`

2. Reload VS Code

### Package and Install

1. Install `vsce` (Visual Studio Code Extension Manager):
   ```bash
   npm install -g @vscode/vsce
   ```

2. Package the extension:
   ```bash
   cd tools/lib/dmod_manifest/vscode-dmm
   vsce package
   ```

3. Install the generated `.vsix` file:
   ```bash
   code --install-extension dmod-manifest-1.0.0.vsix
   ```

## Usage

Once installed, the extension automatically provides syntax highlighting for all files with the `.dmm` extension.

## License

This extension is part of the DMOD project. See the main repository for license information.

## Repository

https://github.com/choco-technologies/dmod
