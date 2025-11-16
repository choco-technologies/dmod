# DMF-GET Tool Documentation

## Overview

The `dmf-get` tool is a package manager for DMOD (Dynamic Modules) that enables downloading and managing DMF packages from manifest files. It was designed to support the DMOD ecosystem by providing a simple, flexible way to distribute and install modules.

## Architecture

The implementation consists of two main components:

### 1. Manifest Parser Library (`libdmod_manifest`)

A standalone static library that handles parsing of `.dmm` (DMOD Manifest) files.

**Location**: `lib/dmod_manifest/`

**Key Features**:
- Parses manifest files from strings, files, or URLs
- Supports comments (`#`) and blank lines
- Handles module entries with optional versions (`module@version url`)
- Processes `$include` directives for recursive manifest inclusion
- Variable substitution for `<tools_name>` and `<arch_name>`
- Pluggable download function via function pointer
- Clean, documented C API

**Design Decisions**:
- Made as a static library for reusability in other projects
- Download function is passed as a function pointer to allow easy replacement
- Uses linked list for storing entries (simple, no memory fragmentation)
- Comprehensive error reporting via `Dmod_Manifest_GetError()`

### 2. DMF-GET Command-Line Tool

The user-facing tool that uses the manifest library to download packages.

**Location**: `tools/system/dmf-get/`

**Key Features**:
- Download modules by name and optional version
- Support for HTTP/HTTPS downloads via libcurl
- Environment variable configuration
- Command-line argument parsing
- Automatic directory creation
- Best-match version selection

## Implementation Details

### Manifest Format

The manifest format supports:

```dmm
# Comments start with #
module_name@version url
module_name url
$include url_to_another_manifest
```

### Variable Substitution

Two variables are automatically substituted in URLs:

1. `<tools_name>` - Direct replacement from config (e.g., `arch/armv7/cortex-m7`)
2. `<arch_name>` - Derived from tools_name:
   - Removes `arch/` prefix if present
   - Replaces `/` with `-`
   - Example: `arch/armv7/cortex-m7` → `armv7-cortex-m7`

### Environment Variables

- `DMOD_TOOLS_NAME` - Tools name for variable substitution
- `DMOD_DMF_DIR` - Default output directory for DMF files
- `DMOD_DMFC_DIR` - Default output directory for DMFC files
- `DMOD_MANIFEST` - Default manifest path or URL

### Version Matching

When searching for a module:
1. Exact version match is preferred
2. If no exact match, returns first available version
3. If no version specified, returns first entry for that module

## Testing

### Unit Tests (20 tests)

Location: `tests/lib/tests_dmod_manifest.cpp`

Coverage:
- Initialization with/without tools name
- Empty manifest and comments-only
- Simple and versioned entries
- Multiple entries parsing
- Variable substitution (tools_name, arch_name, both)
- Include directive (success and failure)
- Entry finding (by name, by version, not found)
- Error handling (null content, out of bounds, invalid lines)

All tests use Google Test framework and pass successfully.

### Integration Tests (5 tests)

Location: `tests/integration/test_dmf_get.sh`

Coverage:
- Help output
- Version output
- Manifest parsing
- Missing module detection
- Variable substitution in manifest

All tests pass successfully.

### CI Integration

Added to `.github/workflows/ci.yml`:
- Install libcurl-dev dependency
- Run manifest library unit tests
- Run dmf-get integration tests

## Dependencies

### Build Dependencies
- CMake 3.18+
- C compiler (GCC/Clang compatible)
- libcurl development package (`libcurl4-openssl-dev`)

### Runtime Dependencies
- libcurl (for HTTP/HTTPS downloads)

## Usage Examples

```bash
# Basic download
dmf-get mymodule

# Specific version
dmf-get mymodule@1.0

# Custom manifest
dmf-get -m https://registry.example.com/manifest.dmm mymodule

# Custom output directory
dmf-get -o /custom/path mymodule

# With tools name for substitution
dmf-get -t arch/armv7/cortex-m7 mymodule
```

## Future Enhancements

### Planned for Future Releases

1. **Dependency Resolution**
   - Parse module metadata from DMF files
   - Recursive dependency tree resolution
   - Circular dependency detection
   - Automatic download of dependencies

2. **Caching**
   - Local cache of downloaded modules
   - Version-based cache keys
   - Cache cleanup utilities

3. **Security**
   - Checksum verification
   - Digital signature support
   - Trust store for publishers

4. **User Experience**
   - Progress bars for downloads
   - Parallel downloads for dependencies
   - Better error messages with suggestions
   - Dry-run mode

5. **Archive Support**
   - Automatic extraction of .zip, .tar.gz files
   - Support for DMP packages

## Technical Notes

### Why Static Library?

The manifest parser is a static library because:
1. It might be useful in other tools (e.g., IDE plugins, CI tools)
2. It has no dependencies on DMOD's runtime (pure C, portable)
3. Easy to test in isolation
4. Can be used in both SYSTEM and MODULE builds

### Why Function Pointer for Downloads?

The download function is passed as a pointer to allow:
1. Easy testing (mock downloads in tests)
2. Future replacement (different HTTP libraries, custom protocols)
3. Network customization (proxies, authentication, retries)
4. No hard dependency on specific network library

### Security Considerations

Current implementation:
- Uses libcurl with SSL/TLS support
- 30-second timeout for downloads
- Follows redirects (limited)
- Basic error checking

Future improvements needed:
- Certificate verification settings
- Checksum/signature verification
- More robust error handling
- Rate limiting

## Development

### Building

```bash
# Configure for SYSTEM mode with tools enabled
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .

# Build
cmake --build build/

# Run tests
./build/tests/tests_dmod_manifest
./tests/integration/test_dmf_get.sh build
```

### Adding New Features

1. Update `dmod_manifest.h` if adding library features
2. Implement in `dmod_manifest.c`
3. Add tests in `tests_dmod_manifest.cpp`
4. Update tool in `tools/system/dmf-get/main.c` if needed
5. Update integration tests
6. Update documentation

## Troubleshooting

### Common Issues

**Issue**: "Could NOT find CURL"
- Solution: Install libcurl-dev: `apt-get install libcurl4-openssl-dev`

**Issue**: "Module not found"
- Check manifest file is valid
- Verify module name and version
- Check environment variables

**Issue**: "Failed to download"
- Check network connectivity
- Verify URL is accessible
- Check firewall/proxy settings

## References

- DMOD Main README: `/README.md`
- Tool README: `/tools/system/dmf-get/README.md`
- Library Header: `/lib/dmod_manifest/dmod_manifest.h`
- Integration Tests: `/tests/integration/test_dmf_get.sh`
