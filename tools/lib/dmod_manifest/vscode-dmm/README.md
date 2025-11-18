# DMOD Manifest Language Support

Syntax highlighting for DMOD Manifest (`.dmm`) files in Visual Studio Code.

## Features

This extension provides syntax highlighting for DMOD manifest files, including:

- **Comments**: Lines starting with `#`
- **DMOD version directive**: `$dmod-version` for specifying required DMOD version
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

# Module without version (version provided at download time)
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

### Variable Placeholders

- `<tools_name>`: Replaced with the tools name (e.g., `arch/armv7/cortex-m7`)
- `<arch_name>`: Replaced with architecture name (e.g., `armv7-cortex-m7`)
- `<version>`: Replaced with the requested module version

## Installation

### From VS Code Marketplace

Search for "DMOD Manifest" in the VS Code Extensions marketplace or install directly:

```bash
code --install-extension ChocoTechnologiesDMOD.dmod-manifest
```

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
   code --install-extension dmod-manifest-1.1.0.vsix
   ```

## Usage

Once installed, the extension automatically provides syntax highlighting for all files with the `.dmm` extension.

## Publishing

### Automated Publishing (Recommended)

The extension is automatically published to the VS Code Marketplace when a new tag is created:

```bash
git tag vscode-dmm-v1.1.0
git push origin vscode-dmm-v1.1.0
```

This triggers the GitHub Actions workflow that packages and publishes the extension.

### Manual Publishing

To manually publish the extension:

1. Install vsce:
   ```bash
   npm install -g @vscode/vsce
   ```

2. Create a Personal Access Token (PAT) from Azure DevOps or GitHub
   - For VS Code Marketplace: https://dev.azure.com/
   - Required scopes: `Marketplace (Manage)`

3. Login to vsce:
   ```bash
   vsce login ChocoTechnologiesDMOD
   ```

4. Publish the extension:
   ```bash
   cd tools/lib/dmod_manifest/vscode-dmm
   vsce publish
   ```

### Version Bumping

Before publishing, update the version in `package.json`:

```bash
# For patch version (1.1.0 -> 1.1.1)
vsce publish patch

# For minor version (1.1.0 -> 1.2.0)
vsce publish minor

# For major version (1.1.0 -> 2.0.0)
vsce publish major
```

## License

This extension is part of the DMOD project. See the main repository for license information.

## Repository

https://github.com/choco-technologies/dmod
