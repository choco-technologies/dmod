# DMOD Resource Language Support

This extension provides syntax highlighting for DMOD Resource (.dmr) files.

## Features

- Syntax highlighting for .dmr files
- Comment support (# for line comments)
- Variable highlighting (${VAR_NAME})
- Special variable support (${destination}, ${module}, ${repo_dir}, ${dmf_dir}, ${build_dir})
- Origin directive highlighting ([origin=path])

## DMR File Format

DMOD Resource files (.dmr) describe what resources should be installed from a zip package and where they should be placed. They can also declare the **origin** of each resource for use when building a release package.

### Syntax

```dmr
# Comments start with #
key=source_path => destination_path
key=source_path => destination_path [origin=path] [origin=path2]
```

### Environment Variables

Variables can be substituted using `${VAR_NAME}` syntax:
- `${destination}` - Installation destination path
- `${module}` - Module name
- `${repo_dir}` - Repository root path (for package creation)
- `${dmf_dir}` - Directory containing built DMF files (for package creation)
- `${build_dir}` - Build output directory (for package creation)
- `${DMOD_DMF_DIR}` - DMF directory from environment
- Any other environment variable

### Origin Directives

The `[origin=path]` directive specifies where files originate from when building a release package. Multiple origins can be listed for a single resource:

```dmr
# Include directory assembled from two source locations
inc=./include => ${destination}/${module}/include [origin=${repo_dir}/include] [origin=${build_dir}/${module}_defs.h]
```

### Example

```dmr
# Install the main module file
dmf=./module.dmf => ${DMOD_DMF_DIR}/${module}.dmf [origin=${dmf_dir}/module.dmf]

# Install dependencies file
dmd=./module.dmd => ${destination}/${module}.dmd

# Install documentation
docs=./module/docs => ${destination}/${module}/docs

# Install headers (from repo include dir and generated headers in build dir)
inc=./module/inc => ${destination}/${module}/inc [origin=${repo_dir}/include] [origin=${build_dir}/${module}_defs.h]

# Install license
license=./LICENSE => ${destination}/${module}/LICENSE
```

## Usage with dmf-get

The `dmf-get` tool automatically looks for `module.dmr` files in zip packages and installs resources according to the mappings.

Use `--mini` flag to install only dmf/dmfc files and skip other resources.

## License

See the main DMOD repository for license information.
