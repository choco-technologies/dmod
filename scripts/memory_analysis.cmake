# ===========================================================================
# Memory Analysis Script for DMOD Libraries
# ===========================================================================
# This script analyzes the memory usage of compiled object files and 
# generates a table showing RAM and ROM usage per source file.
#
# Usage:
#   cmake -P memory_analysis.cmake
#
# Required variables:
#   - OBJECT_FILES: List of object files to analyze
#   - MODULE_NAME: Name of the module being analyzed
#   - ANALYSIS_TYPE: "summary" or "detailed"
# ===========================================================================

# Function to format size in bytes to human-readable format
function(format_size SIZE_BYTES OUTPUT_VAR)
    if(SIZE_BYTES LESS 1024)
        set(${OUTPUT_VAR} "${SIZE_BYTES}B" PARENT_SCOPE)
    elseif(SIZE_BYTES LESS 1048576)
        math(EXPR SIZE_KB "${SIZE_BYTES} / 1024")
        set(${OUTPUT_VAR} "${SIZE_KB}kB" PARENT_SCOPE)
    else()
        math(EXPR SIZE_MB "${SIZE_BYTES} / 1048576")
        set(${OUTPUT_VAR} "${SIZE_MB}MB" PARENT_SCOPE)
    endif()
endfunction()

# Function to parse size output and extract text, data, bss
function(parse_size_output SIZE_OUTPUT TEXT_VAR DATA_VAR BSS_VAR)
    # Size output format: "   text	   data	    bss	    dec	    hex	filename"
    # We need to extract the first three columns
    string(REGEX MATCH "^[ \t]*([0-9]+)[ \t]+([0-9]+)[ \t]+([0-9]+)" MATCH_RESULT "${SIZE_OUTPUT}")
    if(MATCH_RESULT)
        set(${TEXT_VAR} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${DATA_VAR} "${CMAKE_MATCH_2}" PARENT_SCOPE)
        set(${BSS_VAR} "${CMAKE_MATCH_3}" PARENT_SCOPE)
    else()
        set(${TEXT_VAR} "0" PARENT_SCOPE)
        set(${DATA_VAR} "0" PARENT_SCOPE)
        set(${BSS_VAR} "0" PARENT_SCOPE)
    endif()
endfunction()

# Function to print a table row
function(print_table_row FILENAME RAM ROM)
    # Pad filename to 30 characters
    string(LENGTH "${FILENAME}" FILENAME_LEN)
    if(FILENAME_LEN LESS 30)
        math(EXPR PAD_LEN "30 - ${FILENAME_LEN}")
        string(REPEAT " " ${PAD_LEN} PADDING)
        set(FILENAME_PADDED "${FILENAME}${PADDING}")
    else()
        string(SUBSTRING "${FILENAME}" 0 27 FILENAME_PADDED)
        set(FILENAME_PADDED "${FILENAME_PADDED}...")
    endif()
    
    # Pad RAM to 10 characters (right-aligned)
    string(LENGTH "${RAM}" RAM_LEN)
    if(RAM_LEN LESS 10)
        math(EXPR PAD_LEN "10 - ${RAM_LEN}")
        string(REPEAT " " ${PAD_LEN} PADDING)
        set(RAM_PADDED "${PADDING}${RAM}")
    else()
        set(RAM_PADDED "${RAM}")
    endif()
    
    # Pad ROM to 10 characters (right-aligned)
    string(LENGTH "${ROM}" ROM_LEN)
    if(ROM_LEN LESS 10)
        math(EXPR PAD_LEN "10 - ${ROM_LEN}")
        string(REPEAT " " ${PAD_LEN} PADDING)
        set(ROM_PADDED "${PADDING}${ROM}")
    else()
        set(ROM_PADDED "${ROM}")
    endif()
    
    message("  ${FILENAME_PADDED} | ${RAM_PADDED} | ${ROM_PADDED}")
endfunction()

# Main analysis function
function(analyze_memory_usage)
    if(NOT DEFINED OBJECT_FILES OR "${OBJECT_FILES}" STREQUAL "")
        message(WARNING "No object files provided for analysis")
        return()
    endif()
    
    if(NOT DEFINED MODULE_NAME)
        set(MODULE_NAME "Unknown")
    endif()
    
    # Print table header
    message("")
    message("================================================================================")
    message("  Memory Usage Analysis for: ${MODULE_NAME}")
    message("================================================================================")
    message("")
    message("  File                           |        RAM |        ROM")
    message("  -------------------------------+------------+-----------")
    
    set(TOTAL_RAM 0)
    set(TOTAL_ROM 0)
    
    # Process each object file
    # Handle both @@ and space separators
    string(REPLACE "@@" ";" OBJECT_FILES_LIST "${OBJECT_FILES}")
    if("${OBJECT_FILES_LIST}" STREQUAL "${OBJECT_FILES}")
        # No @@ found, try space separator
        string(REPLACE ";" " " OBJECT_FILES_STR "${OBJECT_FILES}")
        separate_arguments(OBJECT_FILES_LIST UNIX_COMMAND "${OBJECT_FILES_STR}")
    endif()
    
    foreach(OBJ_FILE ${OBJECT_FILES_LIST})
        if(NOT EXISTS "${OBJ_FILE}")
            continue()
        endif()
        
        # Get the base filename without path and extension
        get_filename_component(FILENAME "${OBJ_FILE}" NAME)
        string(REGEX REPLACE "\\.c\\.o$" ".c" FILENAME "${FILENAME}")
        
        # Run size command
        execute_process(
            COMMAND size "${OBJ_FILE}"
            OUTPUT_VARIABLE SIZE_OUTPUT
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        # Parse the output (skip header line)
        string(REPLACE "\n" ";" SIZE_LINES "${SIZE_OUTPUT}")
        list(LENGTH SIZE_LINES LINE_COUNT)
        if(LINE_COUNT GREATER 1)
            list(GET SIZE_LINES 1 SIZE_DATA_LINE)
            
            parse_size_output("${SIZE_DATA_LINE}" TEXT DATA BSS)
            
            # Calculate RAM (data + bss) and ROM (text + data)
            math(EXPR RAM "${DATA} + ${BSS}")
            math(EXPR ROM "${TEXT} + ${DATA}")
            
            # Update totals
            math(EXPR TOTAL_RAM "${TOTAL_RAM} + ${RAM}")
            math(EXPR TOTAL_ROM "${TOTAL_ROM} + ${ROM}")
            
            # Format sizes
            format_size(${RAM} RAM_FORMATTED)
            format_size(${ROM} ROM_FORMATTED)
            
            # Print row
            print_table_row("${FILENAME}" "${RAM_FORMATTED}" "${ROM_FORMATTED}")
        endif()
    endforeach()
    
    # Print separator and totals
    message("  -------------------------------+------------+-----------")
    format_size(${TOTAL_RAM} TOTAL_RAM_FORMATTED)
    format_size(${TOTAL_ROM} TOTAL_ROM_FORMATTED)
    print_table_row("TOTAL" "${TOTAL_RAM_FORMATTED}" "${TOTAL_ROM_FORMATTED}")
    message("================================================================================")
    message("")
endfunction()

# Function for detailed symbol analysis
function(analyze_symbols_detailed)
    if(NOT DEFINED OBJECT_FILES OR "${OBJECT_FILES}" STREQUAL "")
        message(WARNING "No object files provided for analysis")
        return()
    endif()
    
    if(NOT DEFINED MODULE_NAME)
        set(MODULE_NAME "Unknown")
    endif()
    
    message("")
    message("================================================================================")
    message("  Detailed Symbol Analysis for: ${MODULE_NAME}")
    message("================================================================================")
    
    # Handle both @@ and space separators
    string(REPLACE "@@" ";" OBJECT_FILES_LIST "${OBJECT_FILES}")
    if("${OBJECT_FILES_LIST}" STREQUAL "${OBJECT_FILES}")
        # No @@ found, try space separator
        string(REPLACE ";" " " OBJECT_FILES_STR "${OBJECT_FILES}")
        separate_arguments(OBJECT_FILES_LIST UNIX_COMMAND "${OBJECT_FILES_STR}")
    endif()
    
    foreach(OBJ_FILE ${OBJECT_FILES_LIST})
        if(NOT EXISTS "${OBJ_FILE}")
            continue()
        endif()
        
        get_filename_component(FILENAME "${OBJ_FILE}" NAME)
        string(REGEX REPLACE "\\.c\\.o$" ".c" FILENAME "${FILENAME}")
        
        message("")
        message("  File: ${FILENAME}")
        message("  " "--------------------------------------------------------------------------------")
        message("  Symbol                         | Type |      Size")
        message("  " "-------------------------------+------+-----------")
        
        # Run nm to get symbol information with sizes
        execute_process(
            COMMAND nm --print-size --size-sort --radix=d "${OBJ_FILE}"
            OUTPUT_VARIABLE NM_OUTPUT
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
        if(NOT "${NM_OUTPUT}" STREQUAL "")
            string(REPLACE "\n" ";" NM_LINES "${NM_OUTPUT}")
            
            foreach(LINE ${NM_LINES})
                # Parse nm output: address size type symbol_name
                string(REGEX MATCH "^[0-9]+ ([0-9]+) ([a-zA-Z]) (.+)$" MATCH_RESULT "${LINE}")
                if(MATCH_RESULT)
                    set(SYMBOL_SIZE "${CMAKE_MATCH_1}")
                    set(SYMBOL_TYPE "${CMAKE_MATCH_2}")
                    set(SYMBOL_NAME "${CMAKE_MATCH_3}")
                    
                    # Format symbol type description
                    if(SYMBOL_TYPE STREQUAL "T" OR SYMBOL_TYPE STREQUAL "t")
                        set(TYPE_DESC "TEXT")
                    elseif(SYMBOL_TYPE STREQUAL "D" OR SYMBOL_TYPE STREQUAL "d")
                        set(TYPE_DESC "DATA")
                    elseif(SYMBOL_TYPE STREQUAL "B" OR SYMBOL_TYPE STREQUAL "b")
                        set(TYPE_DESC "BSS ")
                    elseif(SYMBOL_TYPE STREQUAL "R" OR SYMBOL_TYPE STREQUAL "r")
                        set(TYPE_DESC "RDAT")
                    else()
                        set(TYPE_DESC "${SYMBOL_TYPE}   ")
                    endif()
                    
                    # Pad symbol name to 30 characters
                    string(LENGTH "${SYMBOL_NAME}" NAME_LEN)
                    if(NAME_LEN LESS 30)
                        math(EXPR PAD_LEN "30 - ${NAME_LEN}")
                        string(REPEAT " " ${PAD_LEN} PADDING)
                        set(SYMBOL_NAME_PADDED "${SYMBOL_NAME}${PADDING}")
                    else()
                        string(SUBSTRING "${SYMBOL_NAME}" 0 27 SYMBOL_NAME_PADDED)
                        set(SYMBOL_NAME_PADDED "${SYMBOL_NAME_PADDED}...")
                    endif()
                    
                    # Format size (convert to number first to remove leading zeros)
                    string(REGEX REPLACE "^0+" "" SYMBOL_SIZE_CLEAN "${SYMBOL_SIZE}")
                    if("${SYMBOL_SIZE_CLEAN}" STREQUAL "")
                        set(SYMBOL_SIZE_CLEAN "0")
                    endif()
                    format_size(${SYMBOL_SIZE_CLEAN} SIZE_FORMATTED)
                    string(LENGTH "${SIZE_FORMATTED}" SIZE_LEN)
                    if(SIZE_LEN LESS 10)
                        math(EXPR PAD_LEN "10 - ${SIZE_LEN}")
                        string(REPEAT " " ${PAD_LEN} PADDING)
                        set(SIZE_PADDED "${PADDING}${SIZE_FORMATTED}")
                    else()
                        set(SIZE_PADDED "${SIZE_FORMATTED}")
                    endif()
                    
                    message("  ${SYMBOL_NAME_PADDED} | ${TYPE_DESC} | ${SIZE_PADDED}")
                endif()
            endforeach()
        else()
            message("  No symbols found")
        endif()
    endforeach()
    
    message("")
    message("================================================================================")
    message("")
endfunction()

# Execute the appropriate analysis based on ANALYSIS_TYPE
if(NOT DEFINED ANALYSIS_TYPE)
    set(ANALYSIS_TYPE "summary")
endif()

if(ANALYSIS_TYPE STREQUAL "detailed")
    analyze_symbols_detailed()
else()
    analyze_memory_usage()
endif()
