# DMOD Resource Language Support

This extension provides syntax highlighting for DMOD Resource (.dmr) files.

## Features

- Syntax highlighting for .dmr files
- Comment support (# for line comments)
- Variable highlighting (${VAR_NAME})
- Special variable support (${destination}, ${module})

## DMR File Format

DMOD Resource files (.dmr) describe what resources should be installed from a zip package and where they should be placed.

### Syntax

```dmr
# Comments start with #
key=source_path => destination_path
```

### Environment Variables

Variables can be substituted using `${VAR_NAME}` syntax:
- `${destination}` - Installation destination path
- `${module}` - Module name
- `${DMOD_DMF_DIR}` - DMF directory from environment
- Any other environment variable

### Example

```dmr
# Install the main module file
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf

# Install dependencies file
dmd=./module.dmd => ${destination}/${module}.dmd

# Install documentation
docs=./module/docs => ${destination}/${module}/docs

# Install headers
inc=./module/inc => ${destination}/${module}/inc

# Install license
license=./LICENSE => ${destination}/${module}/LICENSE
```

## Usage with dmf-get

The `dmf-get` tool automatically looks for `module.dmr` files in zip packages and installs resources according to the mappings.

Use `--mini` flag to install only dmf/dmfc files and skip other resources.

## License

See the main DMOD repository for license information.
