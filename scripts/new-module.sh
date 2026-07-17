#!/bin/bash

# ##############################################################################
#
# Script for generating new DMOD modules from templates
#
# Usage:
#   ./scripts/new-module.sh --name MODULE_NAME --type TYPE --path PATH [OPTIONS]
#
# Required parameters:
#   --name NAME       Name of the module
#   --type TYPE       Type of module (library or application)
#   --path PATH       Path to the folder where the module should be created.
#                      May already exist (e.g. a repo just cloned from GitHub
#                      with only README.md/LICENSE in it) - files that already
#                      exist there are left untouched unless --force is given.
#
# Optional parameters:
#   --author AUTHOR   Author name (default: "John Doe")
#   --license LICENSE License name (default: "MIT")
#   --dmod-dir DIR    Path to a local dmod checkout. When set, the generated
#                      CMakeLists.txt is pinned to it (via
#                      FETCHCONTENT_SOURCE_DIR_DMOD) instead of fetching dmod
#                      from GitHub. Also used to fetch Claude Code skills.
#   --github          Generate GitHub Actions workflow
#   --bitbucket       Generate Bitbucket pipeline
#   --dif             Add DIF interface support (library modules only)
#   --mal             Add MAL interface support
#   --port            Add a hardware port module (<name>_port), library
#                      modules only - see dmuart/dmfmc for the pattern
#   --port-arch NAME  Architecture for the generated port skeleton
#                      (default: stm32f7). Requires --port.
#   --force           Overwrite files that already exist at --path
#   --skip-claude-sync Do not run scripts/sync-claude.sh at the end
#   --help            Show this help message
#
# ##############################################################################

set -e

# Default values
AUTHOR_NAME="John Doe"
LICENSE_NAME="MIT"
DMOD_DIR_PATH=""
GENERATE_GITHUB=false
GENERATE_BITBUCKET=false
ADD_DIF=false
ADD_MAL=false
ADD_PORT=false
PORT_ARCH="stm32f7"
FORCE=false
SKIP_CLAUDE_SYNC=false
MODULE_NAME=""
MODULE_TYPE=""
MODULE_PATH=""

# Files actually (over)written this run - the placeholder substitution pass
# only touches these, never the rest of an existing target directory (e.g.
# its .git/ internals or unrelated pre-existing files).
GENERATED_FILES=()

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMOD_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TEMPLATES_DIR="${DMOD_ROOT}/.github/templates"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ##############################################################################
# Helper functions
# ##############################################################################

print_error() {
    echo -e "${RED}Error: $1${NC}" >&2
}

print_success() {
    echo -e "${GREEN}$1${NC}"
}

print_warning() {
    echo -e "${YELLOW}Warning: $1${NC}"
}

print_help() {
    cat << EOF
DMOD Module Generator

Usage:
  ./scripts/new-module.sh --name MODULE_NAME --type TYPE --path PATH [OPTIONS]

Required parameters:
  --name NAME       Name of the module
  --type TYPE       Type of module (library or application)
  --path PATH       Path to the folder where the module should be created.
                     May already exist (e.g. a repo just cloned from GitHub
                     with only README.md/LICENSE in it) - files that already
                     exist there are left untouched unless --force is given.

Optional parameters:
  --author AUTHOR   Author name (default: "John Doe")
  --license LICENSE License name (default: "MIT")
  --dmod-dir DIR    Path to a local dmod checkout. When set, the generated
                     CMakeLists.txt is pinned to it instead of fetching dmod
                     from GitHub. Also used to fetch Claude Code skills.
  --github          Generate GitHub Actions workflow
  --bitbucket       Generate Bitbucket pipeline
  --dif             Add DIF interface support (library modules only)
  --mal             Add MAL interface support
  --port            Add a hardware port module (<name>_port), library
                     modules only
  --port-arch NAME  Architecture for the generated port skeleton
                     (default: stm32f7). Requires --port.
  --force           Overwrite files that already exist at --path
  --skip-claude-sync Do not run scripts/sync-claude.sh at the end
  --help            Show this help message

Examples:
  # Create a simple library module
  ./scripts/new-module.sh --name mylib --type library --path ./modules/mylib

  # Create an application module with custom author
  ./scripts/new-module.sh --name myapp --type application --path ./modules/myapp --author "Jane Smith"

  # Create a library module with DIF interface and GitHub workflow
  ./scripts/new-module.sh --name mylib --type library --path ./modules/mylib --dif --github

  # Create a driver module with a hardware port (like dmuart/dmfmc)
  ./scripts/new-module.sh --name mydriver --type library --path ./modules/mydriver --port --port-arch stm32f7

  # Scaffold into a repo already created on GitHub and cloned locally
  ./scripts/new-module.sh --name myrepo --type application --path . --github

EOF
}

# Insert the contents of block file $2 in place of the line that exactly
# matches literal placeholder $1 in target file $3.
insert_block() {
    local placeholder="$1"
    local block_file="$2"
    local target_file="$3"
    local esc
    esc=$(printf '%s' "${placeholder}" | sed 's/[.[\*^$/]/\\&/g')
    sed -i "/${esc}/{r ${block_file}
d}" "${target_file}"
}

# Remove the line that exactly matches literal placeholder $1 in file $2.
remove_placeholder() {
    local placeholder="$1"
    local target_file="$2"
    local esc
    esc=$(printf '%s' "${placeholder}" | sed 's/[.[\*^$/]/\\&/g')
    sed -i "/${esc}/d" "${target_file}"
}

# Copy $1 to $2, unless $2 already exists and --force was not given (in which
# case it is left untouched and a warning is printed). Tracks every file it
# actually writes in GENERATED_FILES, so later passes (placeholder
# substitution, block insertion) only ever touch files this run produced.
# Returns 0 if the file was written, 1 if it was skipped.
safe_copy() {
    local src="$1"
    local dest="$2"
    if [[ -e "${dest}" && "${FORCE}" != "true" ]]; then
        print_warning "${dest} already exists, skipping (use --force to overwrite)"
        return 1
    fi
    cp "${src}" "${dest}"
    GENERATED_FILES+=("${dest}")
    return 0
}

# ##############################################################################
# Parameter parsing
# ##############################################################################

while [[ $# -gt 0 ]]; do
    case $1 in
        --name)
            MODULE_NAME="$2"
            shift 2
            ;;
        --type)
            MODULE_TYPE="$2"
            shift 2
            ;;
        --path)
            MODULE_PATH="$2"
            shift 2
            ;;
        --author)
            AUTHOR_NAME="$2"
            shift 2
            ;;
        --license)
            LICENSE_NAME="$2"
            shift 2
            ;;
        --dmod-dir)
            DMOD_DIR_PATH="$2"
            shift 2
            ;;
        --github)
            GENERATE_GITHUB=true
            shift
            ;;
        --bitbucket)
            GENERATE_BITBUCKET=true
            shift
            ;;
        --dif)
            ADD_DIF=true
            shift
            ;;
        --mal)
            ADD_MAL=true
            shift
            ;;
        --port)
            ADD_PORT=true
            shift
            ;;
        --port-arch)
            PORT_ARCH="$2"
            shift 2
            ;;
        --force)
            FORCE=true
            shift
            ;;
        --skip-claude-sync)
            SKIP_CLAUDE_SYNC=true
            shift
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            print_error "Unknown parameter: $1"
            print_help
            exit 1
            ;;
    esac
done

# ##############################################################################
# Validation
# ##############################################################################

if [[ -z "${MODULE_NAME}" ]]; then
    print_error "Module name is required"
    print_help
    exit 1
fi

if [[ -z "${MODULE_TYPE}" ]]; then
    print_error "Module type is required"
    print_help
    exit 1
fi

if [[ -z "${MODULE_PATH}" ]]; then
    print_error "Module path is required"
    print_help
    exit 1
fi

# Validate module type
if [[ "${MODULE_TYPE}" != "library" && "${MODULE_TYPE}" != "application" ]]; then
    print_error "Module type must be 'library' or 'application'"
    exit 1
fi

# Validate DIF/MAL/port options
if [[ "${MODULE_TYPE}" == "application" && "${ADD_DIF}" == "true" ]]; then
    print_warning "DIF interfaces are only supported for library modules. Ignoring --dif option."
    ADD_DIF=false
fi

if [[ "${MODULE_TYPE}" == "application" && "${ADD_PORT}" == "true" ]]; then
    print_warning "Hardware ports are only supported for library modules. Ignoring --port option."
    ADD_PORT=false
fi

MODULE_NAME_UPPER=$(printf '%s' "${MODULE_NAME}" | tr '[:lower:]' '[:upper:]')

case "${PORT_ARCH}" in
    stm32f7) DMOD_TOOLS_NAME_VALUE='"arch/armv7/cortex-m7"' ;;
    stm32f4) DMOD_TOOLS_NAME_VALUE='"arch/armv7/cortex-m4"' ;;
    x86_64)  DMOD_TOOLS_NAME_VALUE='"arch/x86_64"' ;;
    *)
        DMOD_TOOLS_NAME_VALUE='"arch/armv7/cortex-m7"'
        if [[ "${ADD_PORT}" == "true" ]]; then
            print_warning "Unknown --port-arch '${PORT_ARCH}' - defaulting DMOD_TOOLS_NAME to arch/armv7/cortex-m7. Adjust src/port/${PORT_ARCH}/config.cmake manually."
        fi
        ;;
esac

# ##############################################################################
# Module creation
# ##############################################################################

echo "Creating DMOD module..."
echo "  Name:   ${MODULE_NAME}"
echo "  Type:   ${MODULE_TYPE}"
echo "  Path:   ${MODULE_PATH}"
echo "  Author: ${AUTHOR_NAME}"
if [[ "${ADD_PORT}" == "true" ]]; then
    echo "  Port:   ${PORT_ARCH}"
fi
echo ""

# Create (or reuse) the module directory. An existing directory is fine -
# e.g. a repo just created on GitHub and cloned locally, containing only
# README.md/LICENSE - individual files are only skipped, not the whole run.
if [[ -e "${MODULE_PATH}" && ! -d "${MODULE_PATH}" ]]; then
    print_error "${MODULE_PATH} exists and is not a directory"
    exit 1
fi

if [[ -d "${MODULE_PATH}" ]]; then
    echo "Using existing directory: ${MODULE_PATH}"
else
    mkdir -p "${MODULE_PATH}"
fi

# Determine template directory
TEMPLATE_SRC="${TEMPLATES_DIR}/${MODULE_TYPE}"

if [[ ! -d "${TEMPLATE_SRC}" ]]; then
    print_error "Template directory not found: ${TEMPLATE_SRC}"
    exit 1
fi

# -----------------------------------------------------------------------------
# CMakeLists.txt
# -----------------------------------------------------------------------------
echo "Generating CMakeLists.txt..."

CMAKELISTS_GENERATED=false
if safe_copy "${TEMPLATE_SRC}/CMakeLists.txt.template" "${MODULE_PATH}/CMakeLists.txt"; then
    CMAKELISTS_GENERATED=true
fi

if [[ "${CMAKELISTS_GENERATED}" == "true" ]]; then
    if [[ "${ADD_PORT}" == "true" ]]; then
        CPU_FAMILY_BLOCK_FILE="$(mktemp)"
        cat > "${CPU_FAMILY_BLOCK_FILE}" << 'EOF'
set(DMOD_CPU_FAMILY "@PORT_ARCH@" CACHE STRING "Target CPU family")
include(${CMAKE_CURRENT_SOURCE_DIR}/src/port/${DMOD_CPU_FAMILY}/config.cmake)

EOF
        insert_block "@PORT_CPU_FAMILY_BLOCK@" "${CPU_FAMILY_BLOCK_FILE}" "${MODULE_PATH}/CMakeLists.txt"
        rm -f "${CPU_FAMILY_BLOCK_FILE}"

        LINK_BLOCK_FILE="$(mktemp)"
        cat > "${LINK_BLOCK_FILE}" << 'EOF'

add_subdirectory(src/port)
target_link_libraries(${DMOD_MODULE_NAME} ${DMOD_MODULE_NAME}_port_if)
EOF
        insert_block "@PORT_LINK_BLOCK@" "${LINK_BLOCK_FILE}" "${MODULE_PATH}/CMakeLists.txt"
        rm -f "${LINK_BLOCK_FILE}"
    elif [[ "${MODULE_TYPE}" == "library" ]]; then
        remove_placeholder "@PORT_CPU_FAMILY_BLOCK@" "${MODULE_PATH}/CMakeLists.txt"
        remove_placeholder "@PORT_LINK_BLOCK@" "${MODULE_PATH}/CMakeLists.txt"
    fi
fi

# -----------------------------------------------------------------------------
# Makefile
# -----------------------------------------------------------------------------
echo "Generating Makefile..."

MAKEFILE_GENERATED=false
if safe_copy "${TEMPLATE_SRC}/Makefile.template" "${MODULE_PATH}/Makefile"; then
    MAKEFILE_GENERATED=true
fi

# @DIF_IMPLS@ / @MAL_IMPLS@ are resolved later, in the shared placeholder pass,
# so both CMakeLists.txt and Makefile get the same value.
if [[ "${ADD_DIF}" == "true" ]]; then
    DIF_IMPLS_VALUE="${MODULE_NAME}"
else
    DIF_IMPLS_VALUE=""
fi

if [[ "${ADD_MAL}" == "true" ]]; then
    MAL_IMPLS_VALUE="${MODULE_NAME}"
else
    MAL_IMPLS_VALUE=""
fi

# -----------------------------------------------------------------------------
# Source, headers, docs, tests
# -----------------------------------------------------------------------------
echo "Generating src/${MODULE_NAME}.c..."

mkdir -p "${MODULE_PATH}/src" "${MODULE_PATH}/docs" "${MODULE_PATH}/tests"
safe_copy "${TEMPLATE_SRC}/main.c.template" "${MODULE_PATH}/src/${MODULE_NAME}.c" || true

if [[ "${MODULE_TYPE}" == "library" ]]; then
    echo "Generating include/${MODULE_NAME}.h..."
    mkdir -p "${MODULE_PATH}/include"
    safe_copy "${TEMPLATE_SRC}/include/module.h.template" "${MODULE_PATH}/include/${MODULE_NAME}.h" || true
fi

echo "Generating docs/..."
safe_copy "${TEMPLATE_SRC}/docs/README.md.template" "${MODULE_PATH}/docs/README.md" || true
safe_copy "${TEMPLATE_SRC}/docs/api-reference.md.template" "${MODULE_PATH}/docs/api-reference.md" || true

echo "Generating tests/..."
safe_copy "${TEMPLATE_SRC}/tests/CMakeLists.txt.template" "${MODULE_PATH}/tests/CMakeLists.txt" || true
safe_copy "${TEMPLATE_SRC}/tests/module_test.c.template" "${MODULE_PATH}/tests/${MODULE_NAME}_test.c" || true

# -----------------------------------------------------------------------------
# manifest.dmm / <module>.dmr
# -----------------------------------------------------------------------------
echo "Generating manifest.dmm..."
MANIFEST_GENERATED=false
if safe_copy "${TEMPLATE_SRC}/manifest.dmm.template" "${MODULE_PATH}/manifest.dmm"; then
    MANIFEST_GENERATED=true
fi

if [[ "${MODULE_TYPE}" == "library" ]]; then
    echo "Generating ${MODULE_NAME}.dmr..."
    safe_copy "${TEMPLATE_SRC}/module.dmr.template" "${MODULE_PATH}/${MODULE_NAME}.dmr" || true
fi

# -----------------------------------------------------------------------------
# README.md
# -----------------------------------------------------------------------------
echo "Generating README.md..."
README_GENERATED=false
if safe_copy "${TEMPLATE_SRC}/README.md.template" "${MODULE_PATH}/README.md"; then
    README_GENERATED=true
fi

if [[ "${README_GENERATED}" == "true" ]]; then
    if [[ "${ADD_PORT}" == "true" ]]; then
        README_PORT_BLOCK_FILE="$(mktemp)"
        cat > "${README_PORT_BLOCK_FILE}" << 'EOF'

## Hardware Port

This module ships two DMOD modules: the architecture-independent
`@MODULE_NAME@` and `@MODULE_NAME@_port`, which contains the
architecture-specific implementation. The active architecture is selected via
`DMOD_CPU_FAMILY` (default: `@PORT_ARCH@`):

```bash
cmake .. -DDMOD_CPU_FAMILY=@PORT_ARCH@
```

See [docs/port-implementation.md](docs/port-implementation.md) for how to add
another architecture. Port-specific files:

```
├── include/@MODULE_NAME@_port.h
├── src/port/
│   ├── CMakeLists.txt
│   └── @PORT_ARCH@/
│       ├── config.cmake
│       └── port.c
└── @MODULE_NAME@_port.dmr
```
EOF
        insert_block "@PORT_README_BLOCK@" "${README_PORT_BLOCK_FILE}" "${MODULE_PATH}/README.md"
        rm -f "${README_PORT_BLOCK_FILE}"
    elif [[ "${MODULE_TYPE}" == "library" ]]; then
        remove_placeholder "@PORT_README_BLOCK@" "${MODULE_PATH}/README.md"
    fi
else
    print_warning "README.md already existed and was left untouched - the \"Hardware Port\"/\"Project Structure\" sections from the template were not added."
fi

# -----------------------------------------------------------------------------
# Hardware port module (library only)
# -----------------------------------------------------------------------------
if [[ "${ADD_PORT}" == "true" ]]; then
    echo "Generating src/port/ (${PORT_ARCH})..."
    PORT_TEMPLATE_SRC="${TEMPLATES_DIR}/port"

    mkdir -p "${MODULE_PATH}/src/port/${PORT_ARCH}"
    safe_copy "${PORT_TEMPLATE_SRC}/src/port/CMakeLists.txt.template" "${MODULE_PATH}/src/port/CMakeLists.txt" || true
    safe_copy "${PORT_TEMPLATE_SRC}/src/port/ARCH/config.cmake.template" "${MODULE_PATH}/src/port/${PORT_ARCH}/config.cmake" || true
    safe_copy "${PORT_TEMPLATE_SRC}/src/port/ARCH/port.c.template" "${MODULE_PATH}/src/port/${PORT_ARCH}/port.c" || true

    safe_copy "${PORT_TEMPLATE_SRC}/include/module_port.h.template" "${MODULE_PATH}/include/${MODULE_NAME}_port.h" || true
    safe_copy "${PORT_TEMPLATE_SRC}/module_port.dmr.template" "${MODULE_PATH}/${MODULE_NAME}_port.dmr" || true
    safe_copy "${PORT_TEMPLATE_SRC}/docs/port-implementation.md.template" "${MODULE_PATH}/docs/port-implementation.md" || true

    if [[ "${MANIFEST_GENERATED}" == "true" ]]; then
        # Add a manifest entry for the port package
        cat >> "${MODULE_PATH}/manifest.dmm" << EOF

# Hardware port package
${MODULE_NAME}_port https://github.com/choco-technologies/${MODULE_NAME}/releases/download/v<version>/${MODULE_NAME}_port-v<version>-<arch_name>.zip
EOF
    else
        print_warning "manifest.dmm already existed - add the ${MODULE_NAME}_port entry to it manually."
    fi
fi

# -----------------------------------------------------------------------------
# Generate CI/CD pipelines
# -----------------------------------------------------------------------------

if [[ "${GENERATE_GITHUB}" == "true" ]]; then
    echo "Generating GitHub Actions workflow..."
    WORKFLOW_DIR="${MODULE_PATH}/.github/workflows"
    mkdir -p "${WORKFLOW_DIR}"

    GITHUB_TEMPLATE="${TEMPLATES_DIR}/ci.yml.template"

    if [[ ! -f "${GITHUB_TEMPLATE}" ]]; then
        print_error "GitHub workflow template not found: ${GITHUB_TEMPLATE}"
        exit 1
    fi

    if safe_copy "${GITHUB_TEMPLATE}" "${WORKFLOW_DIR}/ci.yml"; then
        print_success "GitHub Actions workflow created at ${WORKFLOW_DIR}/ci.yml"
    fi
fi

if [[ "${GENERATE_BITBUCKET}" == "true" ]]; then
    echo "Generating Bitbucket pipeline..."

    BITBUCKET_TEMPLATE="${TEMPLATES_DIR}/bitbucket-pipelines.yml.template"

    if [[ ! -f "${BITBUCKET_TEMPLATE}" ]]; then
        print_error "Bitbucket pipeline template not found: ${BITBUCKET_TEMPLATE}"
        exit 1
    fi

    if safe_copy "${BITBUCKET_TEMPLATE}" "${MODULE_PATH}/bitbucket-pipelines.yml"; then
        print_success "Bitbucket pipeline created at ${MODULE_PATH}/bitbucket-pipelines.yml"
    fi
fi

# -----------------------------------------------------------------------------
# .gitignore
# -----------------------------------------------------------------------------

echo "Generating .gitignore..."

GITIGNORE_TEMPLATE="${TEMPLATES_DIR}/.gitignore.template"

if [[ ! -f "${GITIGNORE_TEMPLATE}" ]]; then
    print_error ".gitignore template not found: ${GITIGNORE_TEMPLATE}"
    exit 1
fi

safe_copy "${GITIGNORE_TEMPLATE}" "${MODULE_PATH}/.gitignore" || true

# -----------------------------------------------------------------------------
# scripts/sync-claude.sh
# -----------------------------------------------------------------------------

echo "Generating scripts/sync-claude.sh..."
mkdir -p "${MODULE_PATH}/scripts"
if safe_copy "${DMOD_ROOT}/scripts/sync-claude.sh" "${MODULE_PATH}/scripts/sync-claude.sh"; then
    chmod +x "${MODULE_PATH}/scripts/sync-claude.sh"
fi

# -----------------------------------------------------------------------------
# Placeholder substitution pass - only over files this run actually wrote
# -----------------------------------------------------------------------------

echo "Substituting placeholders..."

for f in "${GENERATED_FILES[@]}"; do
    sed -i \
        -e "s/@MODULE_NAME_UPPER@/${MODULE_NAME_UPPER}/g" \
        -e "s/@MODULE_NAME@/${MODULE_NAME}/g" \
        -e "s/@AUTHOR_NAME@/${AUTHOR_NAME}/g" \
        -e "s/@LICENSE@/${LICENSE_NAME}/g" \
        -e "s/@PORT_ARCH@/${PORT_ARCH}/g" \
        -e "s/@DIF_IMPLS@/${DIF_IMPLS_VALUE}/g" \
        -e "s/@MAL_IMPLS@/${MAL_IMPLS_VALUE}/g" \
        -e "s|@DMOD_TOOLS_NAME@|${DMOD_TOOLS_NAME_VALUE}|g" \
        "$f"
done

# -----------------------------------------------------------------------------
# --dmod-dir: pin the generated CMakeLists.txt to a local checkout
# -----------------------------------------------------------------------------

if [[ -n "${DMOD_DIR_PATH}" ]]; then
    if [[ "${CMAKELISTS_GENERATED}" == "true" ]]; then
        # Insert after line 1 (cmake_minimum_required must stay the first command)
        sed -i "2i set(DMOD_DIR \"${DMOD_DIR_PATH}\")" "${MODULE_PATH}/CMakeLists.txt"
    fi
    if [[ "${MAKEFILE_GENERATED}" == "true" ]]; then
        sed -i "s|^DMOD_DIR=.*|DMOD_DIR=${DMOD_DIR_PATH}|" "${MODULE_PATH}/Makefile"
    fi
fi

# -----------------------------------------------------------------------------
# Claude Code skills sync
# -----------------------------------------------------------------------------

if [[ "${SKIP_CLAUDE_SYNC}" == "true" ]]; then
    echo "Skipping Claude Code skills sync (--skip-claude-sync)."
else
    echo "Syncing Claude Code skills..."
    SYNC_ARGS=()
    if [[ -n "${DMOD_DIR_PATH}" ]]; then
        SYNC_ARGS+=(--dmod-dir "${DMOD_DIR_PATH}")
    fi
    if ! (cd "${MODULE_PATH}" && ./scripts/sync-claude.sh "${SYNC_ARGS[@]}"); then
        print_warning "Could not sync Claude Code skills (no network / no local dmod checkout). Run ${MODULE_PATH}/scripts/sync-claude.sh manually later."
    fi
fi

# ##############################################################################
# Summary
# ##############################################################################

echo ""
print_success "Module created successfully!"
echo ""
echo "Module details:"
echo "  Location: ${MODULE_PATH}"
echo "  Files written this run: ${#GENERATED_FILES[@]}"
echo "  (any files that already existed were left untouched - see warnings above; re-run with --force to overwrite them)"

echo ""
echo "Next steps:"
echo "  1. Navigate to the module directory: cd ${MODULE_PATH}"
echo "  2. Implement your module logic in src/${MODULE_NAME}.c"
echo "  3. Build the module:"
echo "     - With CMake: mkdir build && cd build && cmake .. && cmake --build ."
echo "     - With Make:  make DMOD_MODE=DMOD_MODULE"
echo ""
