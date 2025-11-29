#!/bin/bash
# =============================================================================
#
#   DMOD Module Debug Script
#
#   This script allows you to debug a module loaded by dmod_loader using GDB.
#   It starts the dmod_loader in background, waits for user to provide the
#   module's base address, and then attaches GDB with symbols loaded at the
#   correct offset.
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

# Default values
GDBSERVER_PORT=1234
USE_GDBSERVER=false
VERBOSE=false

# =============================================================================
#                           Helper Functions
# =============================================================================

print_usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS] <dmod_loader> <module.dmf> <module_elf> <base_address>

Debug a DMOD module loaded by dmod_loader.

ARGUMENTS:
    dmod_loader     Path to the dmod_loader executable
    module.dmf      Path to the DMF module file to load
    module_elf      Path to the ELF file with debug symbols
    base_address    Memory address where the module is loaded (hex, e.g., 0x7f1234567890)

OPTIONS:
    -g, --gdbserver         Use gdbserver instead of direct gdb attachment
    -p, --port PORT         Port for gdbserver (default: 1234)
    -v, --verbose           Enable verbose output
    -h, --help              Show this help message

EXAMPLES:
    # Direct GDB debugging with known base address
    $(basename "$0") ./dmod_loader ./module.dmf ./module 0x7f1234567890

    # Using gdbserver
    $(basename "$0") -g -p 2345 ./dmod_loader ./module.dmf ./module 0x7f1234567890

NOTES:
    - The base_address is the memory address where the module's data is loaded
    - You can obtain this address by adding debug output to your dmod_loader
      that prints the Context->Data pointer value
    - The module_elf is typically the executable built alongside the .dmf file
      (e.g., if your DMF is 'example_app.dmf', the ELF is 'example_app')

WORKFLOW:
    1. Build your module with debug symbols (-g flag)
    2. Find the base address by:
       a. Adding printf("Module base: %p\\n", context->Data); in dmod_loader
       b. Or running dmod_loader once to see where it loads the module
    3. Run this script with the correct base address
    4. GDB will attach with symbols loaded at the correct offset

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

print_verbose() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${BLUE}[DEBUG]${NC} $1"
    fi
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

validate_hex_address() {
    if ! [[ "$1" =~ ^0x[0-9a-fA-F]+$ ]]; then
        print_error "Invalid hex address: $1 (expected format: 0x...)"
        exit 1
    fi
}

# =============================================================================
#                           Main Script
# =============================================================================

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -g|--gdbserver)
            USE_GDBSERVER=true
            shift
            ;;
        -p|--port)
            GDBSERVER_PORT="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
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
if [ $# -lt 4 ]; then
    print_error "Missing required arguments"
    print_usage
    exit 1
fi

DMOD_LOADER="$1"
MODULE_DMF="$2"
MODULE_ELF="$3"
BASE_ADDRESS="$4"

# Validate arguments
check_file_exists "$DMOD_LOADER"
check_executable "$DMOD_LOADER"
check_file_exists "$MODULE_DMF"
check_file_exists "$MODULE_ELF"
validate_hex_address "$BASE_ADDRESS"

print_info "DMOD Module Debug Session"
echo "================================"
print_verbose "dmod_loader: $DMOD_LOADER"
print_verbose "Module DMF:  $MODULE_DMF"
print_verbose "Module ELF:  $MODULE_ELF"
print_verbose "Base Address: $BASE_ADDRESS"

# Check if gdb is available
if ! command -v gdb &> /dev/null; then
    print_error "GDB is not installed. Please install GDB first."
    exit 1
fi

if [ "$USE_GDBSERVER" = true ] && ! command -v gdbserver &> /dev/null; then
    print_error "gdbserver is not installed. Please install gdbserver first."
    exit 1
fi

# Create GDB commands file
GDB_COMMANDS=$(mktemp /tmp/dmod_gdb_commands.XXXXXX)
trap "rm -f $GDB_COMMANDS" EXIT

print_info "Preparing GDB session..."

# Write GDB commands
cat > "$GDB_COMMANDS" << EOF
# DMOD Module Debug Commands
# Generated by dmod-debug.sh

# Set breakpoint on module load (optional, may not work in all setups)
# break Dmod_LoadFile
# break Dmod_Run

# Load symbols from the module ELF at the correct offset
add-symbol-file "$MODULE_ELF" $BASE_ADDRESS

# Print information about loaded symbols
info files

# Set some common breakpoints (user can add more)
echo \n
echo ================================================================================\n
echo   DMOD Module Debugging Session\n
echo ================================================================================\n
echo \n
echo Symbols loaded from: $MODULE_ELF\n
echo Base address: $BASE_ADDRESS\n
echo \n
echo You can now set breakpoints in your module code.\n
echo Common commands:\n
echo   break main              - Break at module's main function\n
echo   break dmod_init         - Break at module's init function\n
echo   break <function_name>   - Break at any function in your module\n
echo   info breakpoints        - List all breakpoints\n
echo   continue                - Continue execution\n
echo   backtrace               - Show call stack\n
echo \n
echo ================================================================================\n
echo \n

EOF

if [ "$USE_GDBSERVER" = true ]; then
    print_info "Starting gdbserver on port $GDBSERVER_PORT..."
    
    # Start gdbserver in background
    gdbserver ":$GDBSERVER_PORT" "$DMOD_LOADER" "$MODULE_DMF" &
    GDBSERVER_PID=$!
    
    sleep 1  # Give gdbserver time to start
    
    print_info "Connecting GDB to gdbserver..."
    
    # Add remote connection command
    cat >> "$GDB_COMMANDS" << EOF
target remote :$GDBSERVER_PORT
EOF
    
    # Run GDB
    gdb -x "$GDB_COMMANDS"
    
    # Kill gdbserver if still running
    kill $GDBSERVER_PID 2>/dev/null || true
else
    # Direct GDB mode - run the program under GDB
    print_info "Starting GDB directly..."
    
    # Add file and run commands
    cat >> "$GDB_COMMANDS" << EOF
# Load the dmod_loader
file "$DMOD_LOADER"

# Set arguments for dmod_loader
set args "$MODULE_DMF"

# Start the program
run
EOF
    
    print_warning "Note: In direct mode, you need to set breakpoints before the module is loaded."
    print_warning "Consider adding a breakpoint in dmod_loader's code after Dmod_LoadFile returns."
    
    # Run GDB
    gdb -x "$GDB_COMMANDS"
fi

print_success "Debug session ended."
