# Add manifest library tests
list(APPEND TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests_dmod_manifest.cpp)

# Add dependencies library tests
list(APPEND TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests_dmod_dependencies.cpp)

set(TEST_SOURCES ${TEST_SOURCES} PARENT_SCOPE)
