
# ===========================================================================
#   Fetch the google test framework
# ===========================================================================
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# ===========================================================================
# 						    Prepare the test sources
# ===========================================================================
enable_testing()
set(TEST_SOURCES "")
set(DMOD_TEST_DMF_FILE "${DMOD_DMF_DIR}/example_app.dmf")
add_definitions(-DDMOD_TEST_DMF_FILE="${DMOD_TEST_DMF_FILE}")

# ===========================================================================
# 						Subdirectories
# ===========================================================================
add_subdirectory(common)
add_subdirectory(module)
add_subdirectory(system)

# ===========================================================================
# 						Test executable
# ===========================================================================
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} ut_main.cpp)
    target_link_libraries(${test_name} gtest gtest_main dmod_system dmod_common dmod_inc)
    add_test(NAME ${test_name} COMMAND ${test_name})

    target_link_options(${test_name} PRIVATE -L ${DMOD_DIR}/scripts)
    target_link_options(${test_name} PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/main.ld)
endforeach()

# Check if TEST DMOD file exists and run the tests if it does
file(GLOB DMF_FILE_FOUND ${DMOD_DMF_DIR}/example_app.dmf)
if(NOT DMF_FILE_FOUND)
    message(WARNING "DMOD_TEST_DMF_FILE not found: ${DMOD_TEST_DMF_FILE} - tests will not be run. Build the DMOD_MODULE first.")
else()
    message(STATUS "DMOD_TEST_DMF_FILE found: ${DMOD_TEST_DMF_FILE}")
    add_custom_target(check ALL COMMAND ctest --output-on-failure DEPENDS ${TEST_SOURCES} ${DMOD_TEST_DMF_FILE} )
endif()