# Find curl
find_package(CURL REQUIRED)

dmod_add_tool(dmf-get
    main.c
)

target_link_libraries(dmf-get
    dmod_manifest
    dmod_dependencies
    dmod_resource
    CURL::libcurl
)

target_include_directories(dmf-get PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/lib/dmod_manifest
    ${CMAKE_SOURCE_DIR}/tools/lib/dmod_dependencies
    ${CMAKE_SOURCE_DIR}/tools/lib/dmod_resource
)
