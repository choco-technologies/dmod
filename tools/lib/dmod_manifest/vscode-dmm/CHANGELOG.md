# Change Log

All notable changes to the "DMOD Manifest Language Support" extension will be documented in this file.

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
