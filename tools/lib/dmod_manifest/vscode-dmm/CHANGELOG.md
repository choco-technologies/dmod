# Change Log

All notable changes to the "DMOD Manifest Language Support" extension will be documented in this file.

## [1.3.0] - 2024-11-27

### Added
- Support for `<cpu_name>` placeholder for CPU-specific module URLs
- Support for `<cpu_family>` placeholder for CPU family-specific module URLs
- Updated example file with `<cpu_name>` and `<cpu_family>` usage

## [1.2.0] - 2024-11-18

### Added
- Support for `$version-available` directive for declaring available module versions
- Syntax highlighting for version-available directive with module name and version list
- Extension icon (blue circular badge with document and "dmm" text)
- Updated example file demonstrating `$version-available` usage
- Documentation of automatic version expansion feature

## [1.1.0] - 2024-11-17

### Added
- Support for `$dmod-version` directive to specify required DMOD version
- Syntax highlighting for DMOD version specifications
- Updated documentation with `$dmod-version` examples and usage
- Documentation of major version compatibility checking

## [1.0.0] - 2025-11-16

### Added
- Initial release
- Syntax highlighting for `.dmm` (DMOD Manifest) files
- Support for comments (lines starting with `#`)
- Highlighting for module entries with optional version (`module@version url`)
- Highlighting for `$include` directives
- Variable placeholder highlighting (`<tools_name>`, `<arch_name>`, `<version>`)
- URL detection and highlighting for HTTP/HTTPS and file:// protocols
- Language configuration for auto-closing pairs and bracket matching
