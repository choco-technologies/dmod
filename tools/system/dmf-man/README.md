# dmf-man - DMOD Documentation Viewer

`dmf-man` is a command-line tool for viewing DMOD module documentation in the terminal. It displays Markdown-formatted documentation with VT100 terminal formatting for enhanced readability.

## Features

- **Markdown Rendering**: Converts Markdown to formatted terminal output using VT100 escape codes
- **Smart Search**: Automatically searches for documentation in multiple standard locations
- **Environment Variable Support**: Respects `DMOD_DOC_DIR` and `DMOD_DMF_DIR` environment variables
- **Flexible Configuration**: Supports custom documentation directory via command-line option

## Installation

Build the tool as part of the DMOD project:

```bash
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/
cmake --install build/  # Optional: install to system
```

The tool will be available at `build/bin/tools/dmf-man`.

## Usage

### Basic Usage

```bash
# View documentation for a module
dmf-man mymodule

# Use custom documentation directory
dmf-man -d /path/to/docs mymodule

# Show help
dmf-man --help

# Show version
dmf-man --version
```

### Command-Line Options

- `-d, --doc-dir <path>` - Path to documentation directory
- `-h, --help` - Show help message
- `-v, --version` - Show version information

### Environment Variables

`dmf-man` respects the following environment variables:

- `DMOD_DOC_DIR` - Primary documentation directory (checked first)
- `DMOD_DMF_DIR` - DMF directory (used as fallback: `<dir>/<module>/docs`)

### Documentation Search Order

`dmf-man` searches for documentation in the following order:

1. **Custom directory** (if specified with `-d`):
   - `<custom_doc_dir>/<module>.md`
   - `<custom_doc_dir>/<module>/README.md`
   - `<custom_doc_dir>/README.md`

2. **DMOD_DOC_DIR** (if set):
   - `$DMOD_DOC_DIR/<module>.md`
   - `$DMOD_DOC_DIR/<module>/README.md`
   - `$DMOD_DOC_DIR/README.md`

3. **DMOD_DMF_DIR** (or `./dmf` as default):
   - `$DMOD_DMF_DIR/<module>/docs/<module>.md`
   - `$DMOD_DMF_DIR/<module>/docs/README.md`
   - `$DMOD_DMF_DIR/<module>/README.md`

### Examples

```bash
# View documentation using default search paths
dmf-man mymodule

# Set documentation directory via environment variable
export DMOD_DOC_DIR=/opt/dmod/docs
dmf-man mymodule

# Use custom documentation directory
dmf-man -d /home/user/projects/dmod-docs mymodule

# Documentation installed by dmf-get
dmf-get docs mymodule  # Downloads docs to $DMOD_DMF_DIR/<module>/docs
dmf-man mymodule       # Will automatically find and display them
```

## Markdown Formatting Support

`dmf-man` supports the following Markdown features with VT100 terminal formatting:

### Headers

```markdown
# Header 1    - Blue, bold, with underline
## Header 2   - Cyan, bold, with underline  
### Header 3  - Green, bold
#### Header 4 - Bold
```

### Text Formatting

- **Bold text**: `**text**` or `__text__` - Rendered in bold
- *Italic text*: `*text*` or `_text_` - Rendered in italic
- `Inline code`: `` `code` `` - Rendered with cyan background
- [Links](url): `[text](url)` - Rendered as underlined text

### Lists

```markdown
- Bullet list item (or * or +)
  Rendered with yellow bullet point

1. Numbered list item
2. Another item
   Rendered with yellow numbers
```

### Code Blocks

````markdown
```
Code block
Multiple lines
```

Or indented with 4 spaces:
    code line 1
    code line 2
````

Rendered in dim color to distinguish from regular text.

### Horizontal Rules

```markdown
---
***
___
```

Rendered as a line across the terminal.

## Integration with dmf-get

`dmf-man` is designed to work seamlessly with `dmf-get`:

1. **Download module documentation** using `dmf-get`:
   ```bash
   dmf-get docs mymodule
   ```

2. **View the documentation** using `dmf-man`:
   ```bash
   dmf-man mymodule
   ```

The documentation will be automatically found in the standard location installed by `dmf-get`.

## Documentation Format

Documentation should be provided as Markdown (`.md`) files. The recommended structure is:

```
<module_name>/
  docs/
    <module_name>.md   # Main documentation
    README.md          # Alternative main documentation
    *.md               # Additional documentation files
```

For more details on packaging documentation with modules, see the [DMR File Format Documentation](../../../docs/dmr-file-format.md).

## Technical Details

- **VT100 Support**: Uses ANSI/VT100 escape codes for terminal formatting
- **Terminal Compatibility**: Works with most modern terminal emulators
- **File Format**: Expects UTF-8 encoded Markdown files
- **Path Safety**: Validates paths to prevent directory traversal

## Limitations

- Does not support all Markdown features (tables, images, etc.)
- Formatting is optimized for terminal display
- Nested formatting may have limitations
- Best viewed in terminals with color support

## Exit Codes

- `0` - Documentation displayed successfully
- `1` - Error occurred (module not found, file not readable, etc.)

## Dependencies

- DMOD library - for system initialization and utilities
- Standard C library - for file operations and string handling

## License

Same as the DMOD project (MIT License).
