cmake_minimum_required(VERSION 3.10)

# Set the project name
project(dmod_loader VERSION ${dmod_VERSION_MAJOR}.${dmod_VERSION_MINOR})

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(${PROJECT_NAME} main.c)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

# Link libraries (if any)
target_link_libraries(${PROJECT_NAME} PRIVATE dmod pthread)

# Statically link system modules (built-in, not distributed as .dmf files) so
# that modules loaded by dmod_loader which rely on their Built-in API (e.g.
# dmlist-backed lists, or the dmosi interface) can resolve at runtime.
dmod_link_builtin(${PROJECT_NAME}
    dmlist
    dmosi
    dmosi-posix
    dmosi-proc
)

target_link_options(${PROJECT_NAME} PRIVATE -L ${DMOD_DIR}/scripts)
target_link_options(${PROJECT_NAME} PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/main.ld)

# ===========================================================================
# Memory Analysis for dmod_loader
# ===========================================================================
# Add custom command to show memory map after build
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
        -DBINARY_FILE=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
        -DMAP_FILE=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
        -DPROJECT_NAME=${PROJECT_NAME}
        -P ${DMOD_SCRIPTS_DIR}/print_memory_map.cmake
    COMMENT "Generating memory map for ${PROJECT_NAME}"
)

# Add linker option to generate map file
target_link_options(${PROJECT_NAME} PRIVATE -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map)

# Add custom target for detailed memory map analysis
add_custom_target(${PROJECT_NAME}_memory_map
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "================================================================================"
    COMMAND ${CMAKE_COMMAND} -E echo "  Detailed Memory Map for: ${PROJECT_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "================================================================================"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map || ${CMAKE_COMMAND} -E echo "Map file not found. Build the project first."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "================================================================================"
    COMMENT "Displaying detailed memory map for ${PROJECT_NAME}"
)

# Install the tool to the system bin directory
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT tools
)
