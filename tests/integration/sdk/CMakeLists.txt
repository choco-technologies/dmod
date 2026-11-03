cmake_minimum_required(VERSION 3.18)
set(sdk_version_args 1.0)
if(DEFINED DMOD_EXPECTED_VERSION)
    string(REGEX REPLACE "-.*$" "" expected_numeric "${DMOD_EXPECTED_VERSION}")
    set(sdk_version_args ${expected_numeric} EXACT)
endif()
find_package(dmod ${sdk_version_args} CONFIG REQUIRED)
project(sdk_consumer LANGUAGES C CXX)
# Finding the same package twice must not create duplicate runtime targets.
find_package(dmod ${sdk_version_args} CONFIG REQUIRED)

set(DMOD_DMR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk_math.dmr")
dmod_add_library(sdk_math "0.1" library.c)
unset(DMOD_DMR_PATH)
target_include_directories(sdk_math_if INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
dmod_add_executable(sdk_app "0.1" main.c)
target_link_libraries(sdk_app sdk_math_if)
dmod_add_test(sdk_test "0.1" test.c)

# SDK settings must not turn unrelated consumer targets into DMOD modules.
add_library(ordinary STATIC ordinary.c)

# A released SDK must preserve the tag's version in package discovery and the
# generated consumer header, including a patch component and prerelease suffix.
if(DEFINED DMOD_EXPECTED_VERSION)
    if(NOT DMOD_RELEASE_VERSION STREQUAL DMOD_EXPECTED_VERSION)
        message(FATAL_ERROR "Installed SDK release version does not match ${DMOD_EXPECTED_VERSION}")
    endif()
    string(REPLACE "." ";" expected_parts "${expected_numeric}")
    list(GET expected_parts 0 expected_major)
    list(GET expected_parts 1 expected_minor)
    list(LENGTH expected_parts expected_count)
    set(expected_patch 0)
    if(expected_count GREATER 2)
        list(GET expected_parts 2 expected_patch)
    endif()
    target_compile_definitions(sdk_app PRIVATE
        DMOD_EXPECTED_VERSION="${DMOD_EXPECTED_VERSION}"
        DMOD_EXPECTED_MAJOR=${expected_major}
        DMOD_EXPECTED_MINOR=${expected_minor}
        DMOD_EXPECTED_PATCH=${expected_patch})
endif()
