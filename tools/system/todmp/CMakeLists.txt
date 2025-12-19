dmod_add_tool(todmp
    main.c
)

target_compile_definitions(todmp PRIVATE DMOD_ENABLE_DMD_SUPPORT)

target_link_libraries(todmp
    dmod_dependencies
)

target_include_directories(todmp PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/lib/dmod_dependencies
)
