# mkdmrpkg - Release Package Directory Maker

## Overview

The `mkdmrpkg` tool reads a `.dmr` resource file and copies files from their
`[origin]` locations into an output directory, reproducing the package structure
defined by the source paths. The resulting directory can then be archived
(e.g. with `zip`) to produce a final release package.

## Usage

```bash
mkdmrpkg <file.dmr> [options]
```

### Arguments

- `<file.dmr>` - Path to the `.dmr` resource file (required)

### Options

| Option | Description |
|--------|-------------|
| `-h`, `--help` | Print help message |
| `-v`, `--version` | Print version information |
| `--verbose` | Enable verbose output |
| `-o <dir>` | Output directory (default: package name or `./package`) |
| `-n`, `--name <name>` | Package name; used as output directory when `-o` is not set |
| `--add-file <path>` | Copy an extra file into the output directory root (repeatable) |
| `-d <destination>` | Value for `${destination}` variable substitution |
| `-m <module>` | Value for `${module}` variable substitution |
| `-r <repo_dir>` | Value for `${repo_dir}` variable substitution |
| `--dmf-dir <dir>` | Value for `${dmf_dir}` variable substitution |
| `--dmfc-dir <dir>` | Value for `${dmfc_dir}` variable substitution |
| `-b <build_dir>` | Value for `${build_dir}` variable substitution |

## Examples

```bash
# Basic usage - output goes to ./package
mkdmrpkg module.dmr -m mymodule

# Custom package name (output goes to ./mymodule-1.0.0/)
mkdmrpkg module.dmr -m mymodule --name mymodule-1.0.0

# Custom package name with explicit output directory
mkdmrpkg module.dmr -m mymodule --name mymodule-1.0.0 -o /tmp/pkg

# Add extra files (e.g. CI-generated release notes)
mkdmrpkg module.dmr -m mymodule -n mymodule-1.0.0 \
  --add-file release-notes.txt \
  --add-file changelog.txt

# Full example with all path substitution variables
mkdmrpkg module.dmr -m mymodule -r /path/to/repo -b /path/to/build \
  --name mymodule-1.0.0
```

## How It Works

1. Parses the `.dmr` resource file and substitutes variables
2. For each entry that has at least one `[origin]` directive:
   - Builds the destination path as `<output_dir>/<entry_source_path>`
   - Copies files or directories from each `[origin]` path to the destination
3. Entries without `[origin]` directives are skipped (informational message)
4. Extra files specified via `--add-file` are copied into the output directory root

## Output Directory Resolution

The output directory is resolved in the following priority order:

1. `-o <dir>` — explicit output path (highest priority)
2. `-n`/`--name <name>` — package name used as directory name
3. Default: `./package`

## See Also

- [mkdmrpkg-tool.md](../../../docs/mkdmrpkg-tool.md) - Full documentation
- [dmr-file-format.md](../../../docs/dmr-file-format.md) - DMR file format reference
- [todmp](../todmp/README.md) - Tool for creating DMP packages from modules
