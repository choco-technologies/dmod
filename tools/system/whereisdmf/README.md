# whereisdmf

`whereisdmf` is a command-line tool for locating DMOD module files in the configured repository directories.

## Description

This tool uses the DMOD API (`Dmod_FindModuleFile`) to search for a module file by name and optionally by architecture. It's useful for:
- Scripting and automation
- Debugging module location issues
- Verifying module installations
- Finding the exact path of a module

## Usage

```bash
whereisdmf <module_name> [arch_name]
```

### Arguments

- `<module_name>` - Name of the module to find (without .dmf/.dmfc extension)
- `[arch_name]` - (optional) Architecture name (default: current system architecture)

### Options

- `-h, --help` - Print help message
- `-v, --version` - Print version information

## Examples

```bash
# Find a module for the current architecture
whereisdmf mymodule

# Find a module for a specific architecture
whereisdmf mymodule x86_64

# Find a module for ARM Cortex-M7
whereisdmf mymodule armv7-cortex-m7

# Use in a script
MODULE_PATH=$(whereisdmf mymodule)
if [ $? -eq 0 ]; then
    echo "Module found at: $MODULE_PATH"
fi
```

## Exit Codes

- `0` - Module found successfully
- `1` - Module not found or error occurred

## How It Works

The tool:
1. Initializes the DMOD system
2. Calls `Dmod_FindModuleFile` with the provided module name and architecture
3. Searches in configured DMOD repository directories (set via `DMOD_REPO_DIR` environment variable)
4. Prints the full path to stdout if found
5. Returns appropriate exit code

## Environment Variables

- `DMOD_REPO_DIR` - Directory containing module files (default: system-specific)

## Building

The tool is built as part of the DMOD project when building system tools:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target whereisdmf
```

## Testing

Integration tests are available in `tests/integration/test_whereisdmf.sh`:

```bash
cd tests/integration
./test_whereisdmf.sh ../../build
```
