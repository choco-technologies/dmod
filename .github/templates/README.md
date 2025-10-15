# DMOD Module Templates

This directory contains templates for creating new DMOD modules using the `scripts/new-module.sh` script.

## Structure

```
.github/templates/
├── library/                        # Library module templates
│   ├── CMakeLists.txt.template     # Internal library CMake config
│   ├── CMakeLists.txt.external.template  # External library CMake config
│   ├── Makefile.template           # Library Makefile
│   └── main.c.template             # Library source template
└── application/                    # Application module templates
    ├── CMakeLists.txt.template     # Internal app CMake config
    ├── CMakeLists.txt.external.template  # External app CMake config
    ├── Makefile.template           # Application Makefile
    └── main.c.template             # Application source template
```

## Template Placeholders

The templates use the following placeholders that are replaced by `scripts/new-module.sh`:

- `@MODULE_NAME@` - Name of the module
- `@AUTHOR_NAME@` - Author name
- `@DMOD_DIR@` - Path to DMOD repository
- `@MAL_IMPLS@` - MAL interface implementations (empty or module name)
- `@DIF_IMPLS@` - DIF interface implementations (empty or module name)

## Usage

These templates are automatically used by the module generation script:

```bash
./scripts/new-module.sh --name mymodule --type library --path ./modules/mymodule
```

The script will:
1. Copy the appropriate template files
2. Replace all placeholders with actual values using `sed`
3. Rename files as needed (e.g., main.c.template → mymodule.c)

## Template Types

### Internal Module Templates
- `CMakeLists.txt.template` - For modules inside the DMOD repository
- Uses relative paths (e.g., `../../..`)

### External Module Templates  
- `CMakeLists.txt.external.template` - For modules outside the DMOD repository
- Includes `dmod_setup_external_module()` call
- Uses absolute DMOD_DIR path

## Editing Templates

To customize module generation:
1. Edit the template files directly in this directory
2. Use `@PLACEHOLDER@` syntax for values that should be replaced
3. Test changes with the module generation script

## See Also

- [Module Generation Script](../../scripts/README.md)
- [Module Templates Documentation](../../templates/module/README.md)
