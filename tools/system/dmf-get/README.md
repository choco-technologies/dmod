# dmf-get - DMOD Package Manager

`dmf-get` is a command-line tool for downloading and managing DMOD packages from manifest files. It serves as a package manager for DMF (DMOD Module Files).

## Features

- **Manifest Parsing**: Read and parse `.dmm` (DMOD Manifest) files
- **Version Management**: Download specific versions of modules
- **Variable Substitution**: Support for `<tools_name>` and `<arch_name>` variables in URLs
- **Include Directives**: Support for `$include` to include other manifests
- **Network Downloads**: Download modules from HTTP/HTTPS URLs using libcurl
- **Flexible Configuration**: Configure via command-line arguments or environment variables

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

```bash
# Download latest version of a module
dmf-get mymodule

# Download specific version
dmf-get mymodule@1.0

# Download multiple modules from a file
dmf-get -f modules.txt

# Use custom manifest
dmf-get -m path/to/manifest.dmm mymodule

# Use custom output directory
dmf-get -o /path/to/output mymodule

# Specify tools name for variable substitution
dmf-get -t arch/armv7/cortex-m7 mymodule
```

### Command-Line Options

- `-f, --file <path>` - File containing list of modules to download (one per line)
- `-m, --manifest <path>` - Path or URL to manifest file
- `-o, --output-dir <path>` - Output directory for downloaded modules
- `-t, --tools-name <name>` - Tools name for variable substitution
- `-a, --arch-name <name>` - Architecture name for variable substitution
- `--type <dmf|dmfc>` - Prefer dmf or dmfc file type
- `--no-dependencies` - Don't download dependencies (not yet implemented)
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

# Download multiple modules from a file
cat > modules.txt << EOF
# Core modules
dmffs
driver@1.0
make_dmffs
EOF
dmf-get -f modules.txt

# Download modules with custom manifest and output directory
dmf-get -m manifest.dmm -o ./output -f modules.txt
```

## Module List File Format

When using the `-f, --file` option, you can provide a text file containing a list of modules to download. The format is simple:

```
# Comment lines start with #
module_name
module_name@version

# Empty lines are ignored
another_module@1.2.3
```

### Example Module List File

```
# Essential DMOD modules
dmffs
driver@1.0
make_dmffs

# Optional modules
utility@2.1
helper
```

### Rules

1. One module per line
2. Comments start with `#` and are ignored
3. Empty lines are ignored
4. Leading and trailing whitespace is trimmed
5. Module specifications follow the same format as command-line arguments (`name` or `name@version`)
6. All modules are processed sequentially, and the tool continues even if some downloads fail
7. A summary is displayed at the end showing successful and failed downloads

## Manifest File Format

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

## Manifest Parser Library

The manifest parsing functionality is implemented as a separate static library (`libdmod_manifest`) that can be used in other projects. See `lib/dmod_manifest/dmod_manifest.h` for the API documentation.

### Library Features

- Parse manifest files from strings, files, or URLs
- Variable substitution
- Recursive include directives
- Best-match version finding
- Pluggable download function (function pointer)

## Testing

Unit tests for the manifest library:
```bash
./build/tests/tests_dmod_manifest
```

Integration tests for dmf-get:
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
