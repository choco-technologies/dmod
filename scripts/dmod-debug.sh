#!/bin/bash
# =============================================================================
#
#   DMOD Module Debug Script
#
#   This script allows you to debug a module loaded by dmod_loader using GDB.
#   It works with dmod_loader's --debug flag which pauses execution after
#   loading the module, allowing you to attach GDB and load symbols at the
#   correct runtime address.
#
#   Copyright (c) 2024 DMOD Contributors
#   MIT License
#
# =============================================================================

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# =============================================================================
#                           Helper Functions
# =============================================================================

print_usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS] <dmod_loader> <module.dmf> <module_elf>

Debug a DMOD module loaded by dmod_loader.

This script helps you debug modules by:
1. Starting dmod_loader with --debug flag (pauses after module load)
2. Guiding you to attach GDB and load symbols at the correct address

ARGUMENTS:
    dmod_loader     Path to the dmod_loader executable
    module.dmf      Path to the DMF module file to load
    module_elf      Path to the ELF file with debug symbols

OPTIONS:
    -h, --help      Show this help message

WORKFLOW:
    1. Run this script with the module paths
    2. The script starts dmod_loader which pauses after loading the module
    3. Note the "Text section" address displayed
    4. In another terminal, run: gdb -p <PID>
    5. In GDB, run: add-symbol-file <module_elf> <text_section_address>
    6. Set your breakpoints: break main (or any function)
    7. Continue: c
    8. Press ENTER in the dmod_loader terminal to resume

EXAMPLE:
    $(basename "$0") ./dmod_loader ./dmf/example_app.dmf ./example_app

    Then in another terminal:
    gdb -p <PID_shown>
    (gdb) add-symbol-file ./example_app 0x443140
    (gdb) break main
    (gdb) c

NOTE: The address changes each run, so always use the address shown by --debug.

EOF
}

print_error() {
    echo -e "${RED}Error: $1${NC}" >&2
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

check_file_exists() {
    if [ ! -f "$1" ]; then
        print_error "File not found: $1"
        exit 1
    fi
}

check_executable() {
    if [ ! -x "$1" ]; then
        print_error "File is not executable: $1"
        exit 1
    fi
}

# =============================================================================
#                           Main Script
# =============================================================================

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -*)
            print_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
        *)
            break
            ;;
    esac
done

# Check remaining arguments
if [ $# -lt 3 ]; then
    print_error "Missing required arguments"
    print_usage
    exit 1
fi

DMOD_LOADER="$1"
MODULE_DMF="$2"
MODULE_ELF="$3"

# Get absolute path to ELF for display
MODULE_ELF_ABS=$(realpath "$MODULE_ELF")

# Validate arguments
check_file_exists "$DMOD_LOADER"
check_executable "$DMOD_LOADER"
check_file_exists "$MODULE_DMF"
check_file_exists "$MODULE_ELF"

# Check if gdb is available
if ! command -v gdb &> /dev/null; then
    print_error "GDB is not installed. Please install GDB first."
    exit 1
fi

print_info "DMOD Module Debug Session"
echo "================================"
echo ""
print_info "Starting dmod_loader with --debug flag..."
print_info "The program will pause after loading the module."
echo ""
print_warning "When you see the debug info, open another terminal and run:"
echo ""
echo -e "  ${GREEN}gdb -p <PID>${NC}              # Attach to the process"
echo -e "  ${GREEN}(gdb) add-symbol-file $MODULE_ELF_ABS <TEXT_ADDR>${NC}"
echo -e "  ${GREEN}(gdb) break main${NC}          # Set breakpoint"
echo -e "  ${GREEN}(gdb) c${NC}                   # Continue in GDB"
echo ""
print_warning "Then press ENTER in this terminal to continue execution."
echo ""
echo "================================"
echo ""

# Run dmod_loader with --debug flag
"$DMOD_LOADER" "$MODULE_DMF" --debug

print_success "Debug session ended."
