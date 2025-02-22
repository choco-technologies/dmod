
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
set(TEST_FILES "")
set(DMOD_TEST_DMF_LIB_FILE "${DMOD_DMF_DIR}/dmodex.dmf")
set(DMOD_TEST_DMF_FILE "${DMOD_DMF_DIR}/example_app.dmf")
set(DMOD_TEST_DMFC_FILE "${DMOD_DMF_DIR}/example_app.dmfc")
add_definitions(-DDMOD_TEST_DMF_FILE="${DMOD_TEST_DMF_FILE}")
add_definitions(-DDMOD_TEST_DMF_LIB_FILE="${DMOD_TEST_DMF_LIB_FILE}")

# ===========================================================================
# 						Subdirectories
# ===========================================================================
add_subdirectory(mocks)
add_subdirectory(common)
add_subdirectory(module)
add_subdirectory(system)

# ===========================================================================
# 						Test executable
# ===========================================================================
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} ut_main.cpp)
    target_link_libraries(${test_name} gtest gtest_main gmock dmod_system dmod_common dmod_inc dmod_fastlz)
    add_test(NAME ${test_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_name})

    list(APPEND TEST_FILES ${test_name})
    if(DMOD_COVERAGE)
        message(STATUS "Code coverage enabled for ${test_name}")
    endif()
endforeach()


# Check if TEST DMOD file exists and run the tests if it does
file(GLOB DMF_FILE_FOUND  ${DMOD_TEST_DMF_FILE})
file(GLOB DMFC_FILE_FOUND ${DMOD_TEST_DMFC_FILE})
if(NOT DMF_FILE_FOUND)
    message(WARNING "DMOD_TEST_DMF_FILE not found: ${DMOD_TEST_DMF_FILE} - tests will not be run. Build the DMOD_MODULE first.")
else()
    message(STATUS "DMOD_TEST_DMF_FILE found: ${DMOD_TEST_DMF_FILE}")
    add_custom_target(check ALL COMMAND ctest --output-on-failure DEPENDS ${TEST_FILES} ${DMOD_TEST_DMF_FILE} )
    if(DMOD_COVERAGE)
        add_dependencies(coverage check)
    endif()
endif()
if(DMFC_FILE_FOUND)
    add_definitions(-DDMOD_TEST_DMFC_FILE="${DMOD_TEST_DMFC_FILE}")
endif()