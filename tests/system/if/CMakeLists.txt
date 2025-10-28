add_executable(tests_dmod_rawmem tests_dmod_rawmem.cpp)
target_link_libraries(tests_dmod_rawmem dmod GTest::gtest GTest::gtest_main)
add_test(NAME tests_dmod_rawmem COMMAND tests_dmod_rawmem)

add_executable(tests_dmod_mock_memory tests_dmod_mock_memory.cpp)
target_link_libraries(tests_dmod_mock_memory dmod GTest::gtest GTest::gtest_main)
add_test(NAME tests_dmod_mock_memory COMMAND tests_dmod_mock_memory)
