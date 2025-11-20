# ===========================================================================
# Memory Map Printer for DMOD Loader
# ===========================================================================
# This script displays the memory map of the dmod_loader executable
#
# Required variables:
#   - BINARY_FILE: Path to the executable
#   - MAP_FILE: Path to the map file
#   - PROJECT_NAME: Name of the project
# ===========================================================================

message("")
message("================================================================================")
message("  Memory Map for: ${PROJECT_NAME} (links with entire project)")
message("================================================================================")
message("")

# Run size command if available
if(EXISTS "${BINARY_FILE}")
    execute_process(
        COMMAND size "${BINARY_FILE}"
        OUTPUT_VARIABLE SIZE_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(SIZE_OUTPUT)
        message("${SIZE_OUTPUT}")
    else()
        message("  Size information not available")
    endif()
else()
    message("  Binary file not found: ${BINARY_FILE}")
endif()

message("")
message("  For detailed memory map, see: ${MAP_FILE}")
message("================================================================================")
message("")
