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

# Determine which template to use
if [[ "${DMOD_DIR_PATH}" != "../../.." ]]; then
    # External module
    CMAKE_TEMPLATE="${TEMPLATES_DIR}/${MODULE_TYPE}/CMakeLists.txt.external.template"
else
    # Internal module
    CMAKE_TEMPLATE="${TEMPLATES_DIR}/${MODULE_TYPE}/CMakeLists.txt.template"
fi

if [[ ! -f "${CMAKE_TEMPLATE}" ]]; then
    print_error "CMake template not found: ${CMAKE_TEMPLATE}"
    exit 1
fi

# Copy and process the template
cp "${CMAKE_TEMPLATE}" "${MODULE_PATH}/CMakeLists.txt"
sed -i "s/@MODULE_NAME@/${MODULE_NAME}/g" "${MODULE_PATH}/CMakeLists.txt"
sed -i "s/@AUTHOR_NAME@/${AUTHOR_NAME}/g" "${MODULE_PATH}/CMakeLists.txt"
sed -i "s|@DMOD_DIR@|${DMOD_DIR_PATH}|g" "${MODULE_PATH}/CMakeLists.txt"

# Copy and process Makefile
echo "Generating Makefile..."

MAKEFILE_TEMPLATE="${TEMPLATES_DIR}/${MODULE_TYPE}/Makefile.template"

if [[ ! -f "${MAKEFILE_TEMPLATE}" ]]; then
    print_error "Makefile template not found: ${MAKEFILE_TEMPLATE}"
    exit 1
fi

# Copy and process the template
cp "${MAKEFILE_TEMPLATE}" "${MODULE_PATH}/Makefile"
sed -i "s/@MODULE_NAME@/${MODULE_NAME}/g" "${MODULE_PATH}/Makefile"
sed -i "s/@AUTHOR_NAME@/${AUTHOR_NAME}/g" "${MODULE_PATH}/Makefile"
sed -i "s|@DMOD_DIR@|${DMOD_DIR_PATH}|g" "${MODULE_PATH}/Makefile"

# Add DIF/MAL interface configuration if requested
if [[ "${ADD_DIF}" == "true" ]]; then
    sed -i "s/@DIF_IMPLS@/${MODULE_NAME}/g" "${MODULE_PATH}/Makefile"
else
    sed -i "s/@DIF_IMPLS@//g" "${MODULE_PATH}/Makefile"
fi

if [[ "${ADD_MAL}" == "true" ]]; then
    sed -i "s/@MAL_IMPLS@/${MODULE_NAME}/g" "${MODULE_PATH}/Makefile"
else
    sed -i "s/@MAL_IMPLS@//g" "${MODULE_PATH}/Makefile"
fi

# Copy and rename main.c to module_name.c
echo "Generating ${MODULE_NAME}.c..."

SOURCE_TEMPLATE="${TEMPLATES_DIR}/${MODULE_TYPE}/main.c.template"

if [[ ! -f "${SOURCE_TEMPLATE}" ]]; then
    print_error "Source template not found: ${SOURCE_TEMPLATE}"
    exit 1
fi

cp "${SOURCE_TEMPLATE}" "${MODULE_PATH}/${MODULE_NAME}.c"

# Copy README.md
echo "Generating README.md..."

README_TEMPLATE="${TEMPLATES_DIR}/README.md.template"

if [[ ! -f "${README_TEMPLATE}" ]]; then
    print_error "README template not found: ${README_TEMPLATE}"
    exit 1
fi

# Prepare usage text based on module type
if [[ "${MODULE_TYPE}" == "application" ]]; then
    USAGE_TEXT="This application module can be loaded and executed using the DMOD loader:\n\n\`\`\`bash\ndmod_loader /path/to/${MODULE_NAME}.dmf\n\`\`\`"
else
    USAGE_TEXT="This library module provides functions that can be used by other modules."
fi

# Copy and process the template
cp "${README_TEMPLATE}" "${MODULE_PATH}/README.md"
sed -i "s/@MODULE_NAME@/${MODULE_NAME}/g" "${MODULE_PATH}/README.md"
sed -i "s/@MODULE_TYPE@/${MODULE_TYPE}/g" "${MODULE_PATH}/README.md"
sed -i "s/@AUTHOR_NAME@/${AUTHOR_NAME}/g" "${MODULE_PATH}/README.md"
sed -i "s/@LICENSE@/${LICENSE_NAME}/g" "${MODULE_PATH}/README.md"
sed -i "s|@USAGE@|${USAGE_TEXT}|g" "${MODULE_PATH}/README.md"

# ##############################################################################
# Generate CI/CD pipelines
# ##############################################################################

if [[ "${GENERATE_GITHUB}" == "true" ]]; then
    echo "Generating GitHub Actions workflow..."
    WORKFLOW_DIR="${MODULE_PATH}/.github/workflows"
    mkdir -p "${WORKFLOW_DIR}"
    
    GITHUB_TEMPLATE="${TEMPLATES_DIR}/ci.yml.template"
    
    if [[ ! -f "${GITHUB_TEMPLATE}" ]]; then
        print_error "GitHub workflow template not found: ${GITHUB_TEMPLATE}"
        exit 1
    fi
    
    cp "${GITHUB_TEMPLATE}" "${WORKFLOW_DIR}/ci.yml"
    
    print_success "GitHub Actions workflow created at ${WORKFLOW_DIR}/ci.yml"
fi

if [[ "${GENERATE_BITBUCKET}" == "true" ]]; then
    echo "Generating Bitbucket pipeline..."
    
    BITBUCKET_TEMPLATE="${TEMPLATES_DIR}/bitbucket-pipelines.yml.template"
    
    if [[ ! -f "${BITBUCKET_TEMPLATE}" ]]; then
        print_error "Bitbucket pipeline template not found: ${BITBUCKET_TEMPLATE}"
        exit 1
    fi
    
    cp "${BITBUCKET_TEMPLATE}" "${MODULE_PATH}/bitbucket-pipelines.yml"
    
    print_success "Bitbucket pipeline created at ${MODULE_PATH}/bitbucket-pipelines.yml"
fi

# ##############################################################################
# Create .gitignore
# ##############################################################################

echo "Generating .gitignore..."

GITIGNORE_TEMPLATE="${TEMPLATES_DIR}/.gitignore.template"

if [[ ! -f "${GITIGNORE_TEMPLATE}" ]]; then
    print_error ".gitignore template not found: ${GITIGNORE_TEMPLATE}"
    exit 1
fi

cp "${GITIGNORE_TEMPLATE}" "${MODULE_PATH}/.gitignore"

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
