dmod_add_tool(mkdmrpkg
    main.c
)

target_link_libraries(mkdmrpkg
    dmod_resource
)

target_include_directories(mkdmrpkg PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/lib/dmod_resource
)
