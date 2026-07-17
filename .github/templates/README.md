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
│   ├── CMakeLists.txt.template       # FetchContent-based CMake config
│   ├── Makefile.template
│   ├── main.c.template
│   ├── module.dmr.template           # -> <module>.dmr
│   ├── manifest.dmm.template
│   ├── README.md.template
│   ├── include/module.h.template     # -> include/<module>.h
│   ├── docs/README.md.template       # -> docs/README.md
│   ├── docs/api-reference.md.template
│   └── tests/CMakeLists.txt.template, tests/module_test.c.template
├── application/                      # Application module templates
│   ├── CMakeLists.txt.template
│   ├── Makefile.template
│   ├── main.c.template
│   ├── manifest.dmm.template         # no .dmr - see dmell precedent
│   ├── README.md.template
│   ├── docs/README.md.template, docs/api-reference.md.template
│   └── tests/CMakeLists.txt.template, tests/module_test.c.template
├── port/                              # Hardware port add-on (--port, library only)
│   ├── src/port/CMakeLists.txt.template
│   ├── src/port/ARCH/config.cmake.template, port.c.template  # ARCH renamed to --port-arch
│   ├── include/module_port.h.template
│   ├── module_port.dmr.template
│   └── docs/port-implementation.md.template
├── .gitignore.template                # shared by both types
├── ci.yml.template                    # --github
└── bitbucket-pipelines.yml.template   # --bitbucket
```

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
