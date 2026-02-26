# mkdmrpkg - Release Package Directory Maker

## Overview

**mkdmrpkg** is a command-line tool that reads a `.dmr` (DMOD Resource) file and
copies files from their `[origin]` locations into an output directory, reproducing
the package structure defined by each entry's source path. The resulting directory
can then be archived (e.g. with `zip`) to produce a final release package.

It is designed for use in CI pipelines to assemble release packages in a reproducible
way, including the ability to set a custom package name and append extra files such
as `release-notes.txt`.

## Usage

```bash
mkdmrpkg <file.dmr> [options]
```

### Arguments

| Argument | Required | Description |
|----------|----------|-------------|
| `<file.dmr>` | Yes | Path to the `.dmr` resource file |

### Options

| Option | Description |
|--------|-------------|
| `-h`, `--help` | Print help message |
| `-v`, `--version` | Print version information |
| `--verbose` | Enable verbose output |
| `-o <dir>` | Output directory (default: package name or `./package`) |
| `-n`, `--name <name>` | Package name; used as output directory when `-o` is not set |
| `--add-file <path>` | Copy an extra file into the output directory root (can be specified multiple times) |
| `-d <destination>` | Value for `${destination}` variable substitution |
| `-m <module>` | Value for `${module}` variable substitution |
| `-r <repo_dir>` | Value for `${repo_dir}` variable substitution |
| `--dmf-dir <dir>` | Value for `${dmf_dir}` variable substitution |
| `--dmfc-dir <dir>` | Value for `${dmfc_dir}` variable substitution |
| `-b <build_dir>` | Value for `${build_dir}` variable substitution |

## Examples

```bash
# Basic usage — output goes to ./package
mkdmrpkg module.dmr -m mymodule

# Custom package name — output goes to ./mymodule-1.0.0/
mkdmrpkg module.dmr -m mymodule --name mymodule-1.0.0

# Both --name and -o provided — -o takes priority
mkdmrpkg module.dmr -m mymodule --name mymodule-1.0.0 -o /tmp/release/pkg

# Append CI-only extra files
mkdmrpkg module.dmr -m mymodule -n mymodule-1.0.0 \
  --add-file release-notes.txt \
  --add-file changelog.txt

# Full example with all substitution variables
mkdmrpkg module.dmr -m mymodule \
  -r /path/to/repo \
  -b /path/to/build \
  --dmf-dir /path/to/dmf \
  --dmfc-dir /path/to/dmfc \
  --name mymodule-1.0.0
```

## Output Directory Resolution

The output directory is resolved in the following priority order:

1. **`-o <dir>`** — explicit output path (highest priority)
2. **`-n`/`--name <name>`** — package name used as the directory name
3. **Default** — `./package`

## Package Name (`--name` / `-n`)

The `--name` option sets the human-readable package name. When `-o` is not
provided, the package name is used directly as the output directory name, making
it easy to produce version-named directories such as `mymodule-1.0.0`:

```bash
mkdmrpkg module.dmr -m mymodule --name mymodule-1.0.0
# Creates: mymodule-1.0.0/
```

## Adding Extra Files (`--add-file`)

The `--add-file` option copies an additional file into the root of the output
directory. It can be specified multiple times (up to 64 files). This is
particularly useful for CI pipelines where certain files (e.g. `release-notes.txt`,
`changelog.txt`, checksums) are generated at build time and should be bundled
into the release package:

```bash
mkdmrpkg module.dmr -m mymodule -n mymodule-1.0.0 \
  --add-file release-notes.txt \
  --add-file checksums.sha256
# Creates:
#   mymodule-1.0.0/release-notes.txt
#   mymodule-1.0.0/checksums.sha256
#   ... (other files from .dmr)
```

## How It Works

1. Parses the `.dmr` resource file and performs variable substitution
2. Creates the output directory
3. For each resource entry in the `.dmr` file:
   - If the entry has no `[origin]` directive — it is **skipped**
   - If the entry has one or more `[origin]` directives:
     - Builds the destination path as `<output_dir>/<entry_source_path>`
     - Copies each origin (file or directory) to the destination
4. Copies all extra files specified via `--add-file` into the output directory root
5. Prints a summary of copied entries, skipped entries, and any errors

## Variable Substitution

The following variables are substituted in path expressions within the `.dmr` file:

| Variable | Option | Description |
|----------|--------|-------------|
| `${module}` | `-m <module>` | Module name |
| `${destination}` | `-d <destination>` | Installation destination path |
| `${repo_dir}` | `-r <repo_dir>` | Repository root directory |
| `${dmf_dir}` | `--dmf-dir <dir>` | Directory containing built DMF files |
| `${dmfc_dir}` | `--dmfc-dir <dir>` | Directory containing built DMFC files |
| `${build_dir}` | `-b <build_dir>` | Build output directory |

Any environment variable can also be referenced with `${VAR_NAME}`.

## Typical CI Workflow

```bash
# 1. Build the module
cmake --build build/

# 2. Generate the package directory
mkdmrpkg module.dmr \
  -m mymodule \
  -r $(pwd) \
  -b build \
  --dmf-dir build/dmf \
  --name mymodule-1.0.0 \
  --add-file release-notes.txt

# 3. Archive the package
zip -r mymodule-1.0.0.zip mymodule-1.0.0/
```

## Integration Tests

Location: `tests/integration/test_mkdmrpkg.sh`

Coverage:
- Help and version output
- Non-existent `.dmr` file handling
- Unknown argument rejection
- Package creation from `.dmr` with `[origin]` entries
- Output directory structure and file content preservation
- Entries without `[origin]` are correctly skipped
- Multiple `[origin]` directives for one entry
- Variable substitution in `.dmr`
- `--name` sets the output directory name (`-n` short form)
- `-o` overrides `--name`
- `--add-file` copies extra files into the output root
- Multiple `--add-file` options
- `--dmfc-dir` variable substitution

## See Also

- [tools-installation.md](tools-installation.md) - Installation guide for all DMOD tools
- [dmr-file-format.md](dmr-file-format.md) - Full `.dmr` resource file format reference
- [todmp-tool](../tools/system/todmp/README.md) - Tool for creating DMP packages
- [mkdmrpkg README](../tools/system/mkdmrpkg/README.md) - Tool-level quick reference
