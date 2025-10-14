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
target_link_libraries(${PROJECT_NAME} dmod)

target_link_options(${PROJECT_NAME} PRIVATE -L ${DMOD_DIR}/scripts)
target_link_options(${PROJECT_NAME} PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/main.ld)
