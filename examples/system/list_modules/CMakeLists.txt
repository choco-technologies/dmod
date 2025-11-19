cmake_minimum_required(VERSION 3.10)

# Set the project name
project(list_modules VERSION ${dmod_VERSION_MAJOR}.${dmod_VERSION_MINOR})

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(${PROJECT_NAME} main.c)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME} dmod)

target_link_options(${PROJECT_NAME} PRIVATE -L ${DMOD_DIR}/scripts)
target_link_options(${PROJECT_NAME} PRIVATE -T ${DMOD_DIR}/examples/system/dmod_loader/main.ld)

# Install the tool to the system bin directory
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT tools
)
