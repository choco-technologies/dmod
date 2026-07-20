# Change Log

All notable changes to the "dmod-dependencies" extension will be documented in this file.

## [1.4.0] - 2026-07-20

### Added
- Support for tagged configuration paths using the `<tag>=config_path` syntax (e.g. `driver=board/x.ini`), used by `dmf-get --config-map` to route configuration files to different destination directories
- Syntax highlighting for the tag name and `=` operator in tagged configuration paths

## [1.2.0] - 2026-02-12

### Added
- Support for module configuration file paths
- Syntax highlighting for configuration paths after module names
- Support for custom destination names in module entries
- Updated documentation with configuration path examples

## [1.1.0] - 2024-11-17

### Added
- Support for version range constraints:
  - Greater than or equal: `module@>=1.0`
  - Less than or equal: `module@<=2.0`
  - Greater than: `module@>1.0`
  - Less than: `module@<2.0`
  - Combined ranges: `module@>=1.0<=2.0`
- Syntax highlighting for comparison operators in version constraints
- Updated documentation with version range examples

## [1.0.0] - 2024-11-17

### Added
- Initial release
- Syntax highlighting for `.dmd` files
- Support for comments (`#`)
- Support for module entries with optional versions (`module@version`)
- Support for `$include` directive
- Support for `$from` directive
- Language configuration for VS Code
