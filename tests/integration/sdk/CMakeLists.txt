cmake_minimum_required(VERSION 3.18)
project(sdk_consumer LANGUAGES C CXX)
find_package(dmod 1.0 CONFIG REQUIRED)
# Finding the same package twice must not create duplicate runtime targets.
find_package(dmod 1.0 CONFIG REQUIRED)

set(DMOD_DMR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk_math.dmr")
dmod_add_library(sdk_math "0.1" library.c)
unset(DMOD_DMR_PATH)
target_include_directories(sdk_math_if INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
dmod_add_executable(sdk_app "0.1" main.c)
target_link_libraries(sdk_app sdk_math_if)
dmod_add_test(sdk_test "0.1" test.c)

# SDK settings must not turn unrelated consumer targets into DMOD modules.
add_library(ordinary STATIC ordinary.c)
