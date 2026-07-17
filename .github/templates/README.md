# DMOD Module Templates

This directory contains templates for creating new DMOD modules using the
`scripts/new-module.sh` script. It is the only template source that script
consumes - it is unrelated to `dmod/templates/`, which is a separate,
internal smoke-test fixture used by `dmod`'s own CI build (see
`templates/module/README.md`); do not confuse the two.

## Structure

```
.github/templates/
├── library/                          # Library module templates
│   ├── CMakeLists.txt.template       # FetchContent-based CMake config, sets DMOD_DMR_PATH
│   ├── Makefile.template
│   ├── main.c.template
│   ├── manifest.dmm.template
│   ├── README.md.template
│   ├── include/module.h.template     # -> include/<module>.h
│   ├── docs/README.md.template       # -> docs/README.md
│   ├── docs/api-reference.md.template
│   └── tests/CMakeLists.txt.template, tests/module_test.c.template
├── application/                      # Application module templates
│   ├── CMakeLists.txt.template       # also sets DMOD_DMR_PATH - same packaging as library
│   ├── Makefile.template
│   ├── main.c.template
│   ├── manifest.dmm.template
│   ├── README.md.template
│   ├── docs/README.md.template, docs/api-reference.md.template
│   └── tests/CMakeLists.txt.template, tests/module_test.c.template
├── port/                              # Hardware port add-on (--port, library only)
│   ├── src/port/CMakeLists.txt.template
│   ├── src/port/ARCH/config.cmake.template, port.c.template  # ARCH renamed to --port-arch
│   ├── include/module_port.h.template
│   ├── module_port.dmr.template
│   ├── docs/port-implementation.md.template
│   ├── ci.yml.template               # -> .github/workflows/ci.yml, replaces the shared one when --port is set
│   └── release.yml.template          # -> .github/workflows/release.yml, replaces the shared one when --port is set
├── module.dmr.template                # -> <module>.dmr, shared by both types (DMOD_DMR_PATH-driven packaging)
├── release.yml.template               # -> .github/workflows/release.yml, shared by both types (--github, no --port)
├── .gitignore.template                # shared by both types
├── ci.yml.template                    # --github, library/application without --port
└── bitbucket-pipelines.yml.template   # --bitbucket
```

`module.dmr.template` needs no `@MODULE_NAME@` substitution at all - it's
written entirely in terms of the `.dmr` format's own `${module}` built-in
variable (resolved by `dmf-get`/`mkdmrpkg`, not by `new-module.sh`'s `sed`
pass), which is why the same file works unmodified for every module. It also
adds itself (`dmr=./${module}.dmr => ...`) - keep that entry if you ever edit
this file, it's easy to forget.

Setting `DMOD_DMR_PATH` in `CMakeLists.txt` (both `library` and `application`
templates do this) is what makes `dmod_add_library`/`dmod_add_executable`
assemble `build/packages/<module>/` automatically via `todmp`/`mkdmrpkg` -
`release.yml.template` just adds `RELEASE_NOTES.txt` to that directory and
zips it. Don't hand-copy `.dmf`/`.dmfc` files in a release workflow; if you
find yourself doing that, `DMOD_DMR_PATH` is probably missing or the `.dmr` is
incomplete. See `dmuart/dmuart.dmr` and `dmuart/CMakeLists.txt` for the
real-world reference this pattern is copied from.

Both `ci.yml.template` and every `release.yml.template` pin the CI container to
`chocotechnologies/dmod:<version>` - keep this in sync with the version used
by `dmod`'s own `.github/workflows/ci.yml` and the other module repos
(currently `1.0.4`) when it changes.

`release.yml.template` (and `port/release.yml.template`) never hardcode a
module name for packaging: they loop over every subdirectory found under
`build_.../packages/` after the build, zip each one, and write their names to
`package-names.txt` for `generate-versions-manifest` to turn into
`$version-available` lines. This is what makes the release flow keep working
if a repo grows more than one module (a core+port split already does this,
but nothing stops a repo from defining further modules by hand).

`library/main.c.template`'s example interface uses dmod's Built-in API macro
pair - `dmod_@MODULE_NAME@_api(...)` to declare in the header,
`dmod_@MODULE_NAME@_api_declaration(...)` to define in the `.c` file - and
`Dmod_Malloc`/`Dmod_Free` (dmod's SAL) instead of plain C prototypes and
`malloc`/`free`. This isn't stylistic: a plain C function declared in a
module's public header will not be resolved by the loader (or link against
`libc`, which isn't guaranteed to be linked into a module on an embedded
target) - see `dm_sw_ring/include/dm_sw_ring.h` + `dm_sw_ring/src/dm_sw_ring.c`
for the real-world pattern this was copied from.

## Template Placeholders

The templates use the following placeholders that are replaced by
`scripts/new-module.sh` in a single pass over every generated file:

- `@MODULE_NAME@` - Name of the module
- `@MODULE_NAME_UPPER@` - Uppercase module name (used in header guards)
- `@AUTHOR_NAME@` - Author name
- `@LICENSE@` - License name
- `@MAL_IMPLS@` - MAL interface implementations (empty or module name)
- `@DIF_IMPLS@` - DIF interface implementations (empty or module name, library only)
- `@PORT_ARCH@` - Architecture name passed via `--port-arch` (default `stm32f7`)
- `@DMOD_TOOLS_NAME@` - `DMOD_TOOLS_NAME` value mapped from `--port-arch`

Two block placeholders are resolved before the substitution pass, by either
inserting a generated multi-line block or deleting the placeholder line
entirely, depending on whether `--port` was passed:

- `@PORT_CPU_FAMILY_BLOCK@`, `@PORT_LINK_BLOCK@` (in `library/CMakeLists.txt.template`)
- `@PORT_README_BLOCK@` (in `library/README.md.template`)

## Usage

These templates are automatically used by the module generation script:

```bash
./scripts/new-module.sh --name mymodule --type library --path ./modules/mymodule
```

The script will:
1. Copy the appropriate template files (library/application, plus `port/` when `--port` is given)
2. Replace all placeholders with actual values using `sed`
3. Rename files as needed (e.g. `main.c.template` → `src/mymodule.c`)
4. Run `scripts/sync-claude.sh` to populate `.claude/skills/` (unless `--skip-claude-sync`)

## Editing Templates

To customize module generation:
1. Edit the template files directly in this directory
2. Use `@PLACEHOLDER@` syntax for values that should be replaced
3. Test changes with the module generation script (see verification steps in
   `scripts/README.md`)

## See Also

- [Module Generation Script](../../scripts/README.md)
- [CMake Functions Reference](../../docs/cmake-functions.md)
