# DMOD Manifest Language Support

Syntax highlighting for DMOD Manifest (`.dmm`) files in Visual Studio Code.

## Features

This extension provides syntax highlighting for DMOD manifest files, including:

- **Comments**: Lines starting with `#`
- **Module entries**: `module@version url` syntax
- **Include directives**: `$include url` for manifest inclusion
- **Variable placeholders**: `<tools_name>`, `<arch_name>`, `<version>`
- **URLs**: HTTP/HTTPS and file:// URLs

## Manifest Format

DMOD manifest files (`.dmm`) define package modules with the following syntax:

```dmm
# This is a comment

# Module with version
mymodule@1.0 https://registry.example.com/modules/mymodule.dmf

# Module without version (version provided at download time)
dmffs https://github.com/example/releases/download/v<version>/dmffs-<arch_name>.zip

# Include another manifest
$include https://registry.example.com/additional-manifest.dmm

# Local file reference
localmodule@2.0 /data/modules/localmodule.dmfc
```

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
