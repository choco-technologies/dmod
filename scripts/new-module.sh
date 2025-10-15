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
#   --path PATH       Path to the folder where the module should be created
#
# Optional parameters:
#   --author AUTHOR   Author name (default: "John Doe")
#   --license LICENSE License name (default: "MIT")
#   --dmod-dir DIR    Path to DMOD repository (default: auto-detect or ../../..)
#   --github          Generate GitHub Actions workflow
#   --bitbucket       Generate Bitbucket pipeline
#   --dif             Add DIF interface support (library modules only)
#   --mal             Add MAL interface support
#   --help            Show this help message
#
# ##############################################################################

set -e

# Default values
AUTHOR_NAME="John Doe"
LICENSE_NAME="MIT"
DMOD_DIR_PATH="../../.."
GENERATE_GITHUB=false
GENERATE_BITBUCKET=false
ADD_DIF=false
ADD_MAL=false
MODULE_NAME=""
MODULE_TYPE=""
MODULE_PATH=""

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMOD_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TEMPLATES_DIR="${DMOD_ROOT}/templates/module"

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
  --path PATH       Path to the folder where the module should be created

Optional parameters:
  --author AUTHOR   Author name (default: "John Doe")
  --license LICENSE License name (default: "MIT")
  --dmod-dir DIR    Path to DMOD repository (default: ../../..)
  --github          Generate GitHub Actions workflow
  --bitbucket       Generate Bitbucket pipeline
  --dif             Add DIF interface support (library modules only)
  --mal             Add MAL interface support
  --help            Show this help message

Examples:
  # Create a simple library module
  ./scripts/new-module.sh --name mylib --type library --path ./modules/mylib

  # Create an application module with custom author
  ./scripts/new-module.sh --name myapp --type application --path ./modules/myapp --author "Jane Smith"

  # Create a library module with DIF interface and GitHub workflow
  ./scripts/new-module.sh --name mylib --type library --path ./modules/mylib --dif --github

EOF
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

# Validate DIF/MAL options
if [[ "${MODULE_TYPE}" == "application" && "${ADD_DIF}" == "true" ]]; then
    print_warning "DIF interfaces are only supported for library modules. Ignoring --dif option."
    ADD_DIF=false
fi

# ##############################################################################
# Module creation
# ##############################################################################

echo "Creating DMOD module..."
echo "  Name:   ${MODULE_NAME}"
echo "  Type:   ${MODULE_TYPE}"
echo "  Path:   ${MODULE_PATH}"
echo "  Author: ${AUTHOR_NAME}"
echo ""

# Create module directory
if [[ -d "${MODULE_PATH}" ]]; then
    print_error "Directory ${MODULE_PATH} already exists"
    exit 1
fi

mkdir -p "${MODULE_PATH}"

# Determine template directory
TEMPLATE_SRC="${TEMPLATES_DIR}/${MODULE_TYPE}"

if [[ ! -d "${TEMPLATE_SRC}" ]]; then
    print_error "Template directory not found: ${TEMPLATE_SRC}"
    exit 1
fi

# Copy and process CMakeLists.txt
echo "Generating CMakeLists.txt..."

# For external modules (not in the DMOD tree), add DMOD setup
if [[ "${DMOD_DIR_PATH}" != "../../.." ]]; then
    # This is an external module
    cat > "${MODULE_PATH}/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.18)

set(DMOD_DIR "${DMOD_DIR_PATH}")
set(DMOD_MODE "DMOD_MODULE" CACHE STRING "Mode" FORCE)
set(DMOD_BUILD_DIR "\${CMAKE_CURRENT_BINARY_DIR}")

include(\${DMOD_DIR}/paths.cmake)

project(${MODULE_NAME}_module)

dmod_setup_external_module()

# Name of the module 
set(DMOD_MODULE_NAME        ${MODULE_NAME})

# Version (should be string in format "Major.Minor") 
set(DMOD_MODULE_VERSION     "0.1")

# Author (should be string)
set(DMOD_AUTHOR_NAME        ${AUTHOR_NAME})

# Stack size for the module (should be integer)
set(DMOD_STACK_SIZE         1024)

EOF

    if [[ "${MODULE_TYPE}" == "library" ]]; then
        echo "#" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   dmod_add_library - create a library module" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   it has the same signature as add_library" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   and can be used in the same way after the creation" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   (for example, to link libraries)" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "dmod_add_library(\${DMOD_MODULE_NAME} \"0.1\"" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "    # List of source files - can include C and C++ files" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "    ${MODULE_NAME}.c" >> "${MODULE_PATH}/CMakeLists.txt"
        echo ")" >> "${MODULE_PATH}/CMakeLists.txt"
    else
        echo "# Priority for the thread of the module (should be integer)" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "set(DMOD_PRIORITY           0)" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "# " >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   dmod_add_executable - create an executable module" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   it has the same signature as add_executable, " >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   and can be used in the same way after the creation" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#   (for example, to link libraries)" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "#" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "dmod_add_executable(\${DMOD_MODULE_NAME} \"0.1\"" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "    # List of source files - can include C and C++ files" >> "${MODULE_PATH}/CMakeLists.txt"
        echo "    ${MODULE_NAME}.c" >> "${MODULE_PATH}/CMakeLists.txt"
        echo ")" >> "${MODULE_PATH}/CMakeLists.txt"
    fi
else
    # This is an internal module (within DMOD tree)
    sed -e "s/@DMOD_MODULE_NAME@/${MODULE_NAME}/g" \
        -e "s/@DMOD_AUTHOR_NAME@/${AUTHOR_NAME}/g" \
        -e "s/@DMOD_STACK_SIZE@/1024/g" \
        "${TEMPLATE_SRC}/CMakeLists.txt" > "${MODULE_PATH}/CMakeLists.txt"
    
    # Update the source file reference in CMakeLists.txt
    sed -i "s/@DMOD_MODULE_NAME@\.c/${MODULE_NAME}.c/g" "${MODULE_PATH}/CMakeLists.txt"
fi

# Copy and process Makefile
echo "Generating Makefile..."
sed -e "s/@MODULE_NAME@/${MODULE_NAME}/g" \
    -e "s/@AUTHOR_NAME@/${AUTHOR_NAME}/g" \
    "${TEMPLATE_SRC}/Makefile" > "${MODULE_PATH}/Makefile"

# Update the source file reference in Makefile
sed -i "s/DMOD_CSOURCES=main.c/DMOD_CSOURCES=${MODULE_NAME}.c/g" "${MODULE_PATH}/Makefile"

# Update DMOD_DIR in Makefile
sed -i "s|DMOD_DIR=../../..|DMOD_DIR=${DMOD_DIR_PATH}|g" "${MODULE_PATH}/Makefile"

# Add DIF/MAL interface configuration to Makefile if requested
if [[ "${ADD_DIF}" == "true" ]]; then
    sed -i "s/DMOD_DIF_IMPLS=/DMOD_DIF_IMPLS=${MODULE_NAME}/g" "${MODULE_PATH}/Makefile"
fi

if [[ "${ADD_MAL}" == "true" ]]; then
    sed -i "s/DMOD_MAL_IMPLS=/DMOD_MAL_IMPLS=${MODULE_NAME}/g" "${MODULE_PATH}/Makefile"
fi

# Copy and rename main.c to module_name.c
echo "Generating ${MODULE_NAME}.c..."
cp "${TEMPLATE_SRC}/main.c" "${MODULE_PATH}/${MODULE_NAME}.c"

# Copy README.md
echo "Generating README.md..."
sed -e "s/Example Library/${MODULE_NAME}/g" \
    -e "s/Example Application/${MODULE_NAME}/g" \
    "${TEMPLATE_SRC}/README.md" > "${MODULE_PATH}/README.md"

# Update README with actual module name and description
cat > "${MODULE_PATH}/README.md" << EOF
# ${MODULE_NAME}

DMOD ${MODULE_TYPE} module.

## Description

This is a ${MODULE_TYPE} module for the DMOD system.

## Author

${AUTHOR_NAME}

## License

${LICENSE_NAME}

## Building

### Using CMake

\`\`\`bash
mkdir -p build
cd build
cmake .. -DDMOD_MODE=DMOD_MODULE
cmake --build .
\`\`\`

### Using Make

\`\`\`bash
make DMOD_MODE=DMOD_MODULE
\`\`\`

## Usage

EOF

if [[ "${MODULE_TYPE}" == "application" ]]; then
    cat >> "${MODULE_PATH}/README.md" << EOF
This application module can be loaded and executed using the DMOD loader:

\`\`\`bash
dmod_loader /path/to/${MODULE_NAME}.dmf
\`\`\`
EOF
else
    cat >> "${MODULE_PATH}/README.md" << EOF
This library module provides functions that can be used by other modules.
EOF
fi

# ##############################################################################
# Generate CI/CD pipelines
# ##############################################################################

if [[ "${GENERATE_GITHUB}" == "true" ]]; then
    echo "Generating GitHub Actions workflow..."
    WORKFLOW_DIR="${MODULE_PATH}/.github/workflows"
    mkdir -p "${WORKFLOW_DIR}"
    
    cat > "${WORKFLOW_DIR}/ci.yml" << 'EOF'
name: CI

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  build:
    name: Build and Test
    runs-on: ubuntu-latest
    container:
      image: chocotechnologies/dmod:1.0.1
    
    steps:
      - name: Checkout code
        uses: actions/checkout@v4
      
      - name: Build with CMake
        run: |
          mkdir -p build
          cd build
          cmake .. -DDMOD_MODE=DMOD_MODULE
          cmake --build .
      
      - name: Build with Make
        run: |
          make clean
          make DMOD_MODE=DMOD_MODULE
EOF
    
    print_success "GitHub Actions workflow created at ${WORKFLOW_DIR}/ci.yml"
fi

if [[ "${GENERATE_BITBUCKET}" == "true" ]]; then
    echo "Generating Bitbucket pipeline..."
    
    cat > "${MODULE_PATH}/bitbucket-pipelines.yml" << 'EOF'
image: chocotechnologies/dmod:1.0.1

pipelines:
  default:
    - step:
        name: Build with CMake
        script:
          - mkdir -p build
          - cd build
          - cmake .. -DDMOD_MODE=DMOD_MODULE
          - cmake --build .
    
    - step:
        name: Build with Make
        script:
          - make clean
          - make DMOD_MODE=DMOD_MODULE
EOF
    
    print_success "Bitbucket pipeline created at ${MODULE_PATH}/bitbucket-pipelines.yml"
fi

# ##############################################################################
# Create .gitignore
# ##############################################################################

echo "Generating .gitignore..."
cat > "${MODULE_PATH}/.gitignore" << 'EOF'
# Build directories
build/
*.dmf
*.dmfc

# IDE
.vscode/
.idea/

# Temporary files
*.tmp
*.swp
*~
EOF

# ##############################################################################
# Summary
# ##############################################################################

echo ""
print_success "Module created successfully!"
echo ""
echo "Module details:"
echo "  Location: ${MODULE_PATH}"
echo "  Files created:"
echo "    - CMakeLists.txt"
echo "    - Makefile"
echo "    - ${MODULE_NAME}.c"
echo "    - README.md"
echo "    - .gitignore"

if [[ "${GENERATE_GITHUB}" == "true" ]]; then
    echo "    - .github/workflows/ci.yml"
fi

if [[ "${GENERATE_BITBUCKET}" == "true" ]]; then
    echo "    - bitbucket-pipelines.yml"
fi

echo ""
echo "Next steps:"
echo "  1. Navigate to the module directory: cd ${MODULE_PATH}"
echo "  2. Implement your module logic in ${MODULE_NAME}.c"
echo "  3. Build the module:"
echo "     - With CMake: mkdir build && cd build && cmake .. -DDMOD_MODE=DMOD_MODULE && cmake --build ."
echo "     - With Make:  make DMOD_MODE=DMOD_MODULE"
echo ""
