# todmfc - DMF Compression Tool

## Overview

`todmfc` (to DMFC) is a command-line tool for compressing DMF (DMOD Module Files) into DMFC format. The DMFC format is a compressed version of DMF files that reduces storage size and transfer time while maintaining full compatibility with the DMOD system.

## Features

- **Compression of DMF files**: Converts standard DMF files to compressed DMFC format
- **Multiple compression methods**: Supports various compression algorithms (currently: fastlz)
- **Configurable compression levels**: Allows tuning between compression ratio and speed
- **Size comparison**: Displays original and compressed file sizes with compression ratio
- **Simple interface**: Easy-to-use command-line tool

## Usage

### Basic Usage

```bash
todmfc path/to/file.dmf path/to/output.dmfc
```

This will compress the DMF file using the default compression method (fastlz) and level (2).

### Advanced Usage

```bash
todmfc path/to/file.dmf path/to/output.dmfc [compression_method] [level]
```

**Parameters:**
- `path/to/file.dmf` - Input DMF file to compress
- `path/to/output.dmfc` - Output DMFC file path
- `[compression_method]` - (Optional) Compression algorithm to use (default: fastlz)
- `[level]` - (Optional) Compression level (default: 2)

### Options

```bash
todmfc -h, --help       # Display help message
todmfc -v, --version    # Display version information
```

## Examples

### Compress with default settings

```bash
todmfc mymodule.dmf mymodule.dmfc
```

### Compress with specific method and level

```bash
todmfc mymodule.dmf mymodule.dmfc fastlz 3
```

### Display help

```bash
todmfc --help
```

## Compression Methods

The tool supports different compression methods. To see available compression methods on your system, run:

```bash
todmfc --help
```

Currently supported:
- **fastlz** - Fast compression algorithm with good compression ratios

## Compression Levels

Compression levels affect the trade-off between compression ratio and processing time:
- **Lower levels (1)**: Faster compression, lower compression ratio
- **Higher levels (3+)**: Better compression ratio, slower processing

The optimal level depends on your use case. Level 2 (default) provides a good balance.

## Output

After successful compression, `todmfc` displays:
- Confirmation message
- Original file size
- Compressed file size
- Compression ratio (percentage)

Example output:
```
DMF file 'mymodule.dmf' was successfully converted to DMFC file 'mymodule.dmfc'
Original file size: 245760 bytes
Compressed file size: 89432 bytes
Compression ratio: 36.40%
```

## When to Use DMFC

Use DMFC files when:
- **Storage is limited**: Reduce flash memory usage on embedded systems
- **Transfer time matters**: Faster downloads and deployments
- **Distribution**: Smaller files for package distribution

Use regular DMF files when:
- **Fast loading required**: Skip decompression overhead
- **Development**: Easier debugging with uncompressed files

## Integration with DMOD

DMFC files are fully compatible with the DMOD system. The loader automatically detects and decompresses DMFC files during module loading. No code changes are needed to use DMFC instead of DMF files.

## Building

### Prerequisites
- DMOD library
- Standard C compiler (GCC or compatible)
- Make or CMake

### Build with Make

```bash
cd tools/system/todmfc
make
```

### Build with CMake

```bash
cd tools/system/todmfc
cmake -B build -S .
cmake --build build
```

## Installation

### System-wide installation

```bash
cd tools/system/todmfc
sudo make install
```

By default, installs to `/usr/local/bin`. Use `INSTALL_PREFIX` to change:

```bash
sudo make install INSTALL_PREFIX=/custom/path
```

### Uninstalling

```bash
cd tools/system/todmfc
sudo make uninstall
```

## Error Handling

Common errors and solutions:

**"Too few arguments"**
- Provide both input DMF and output DMFC file paths

**"Compression method 'X' is not supported"**
- Use `--help` to see available compression methods

**"Failed to convert DMF to DMFC"**
- Check input file exists and is a valid DMF file
- Verify write permissions for output directory
- Ensure sufficient disk space

## See Also

- [DMOD Tools Installation Guide](../../../docs/tools-installation.md) - Complete guide for all DMOD tools
- [todmp](../todmp/README.md) - DMP package creator for bundling multiple modules
- [dmf-get](../dmf-get/README.md) - DMOD package manager

## License

Part of the DMOD framework. See [license.md](../../../license.md) for details.
