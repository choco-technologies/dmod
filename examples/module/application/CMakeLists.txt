set(DMOD_MODULE_NAME        example_app)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        "Patryk Kubiak")
set(DMOD_STACK_SIZE         1024)
set(DMOD_PRIORITY           0)

# ##############################################################################
# 
#   DMOD_MAL_IMPLS
#       
#           List of MAL interfaces implemented by the module
#
# ##############################################################################
set(DMOD_MAL_IMPLS
    example_lib
)

dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
    example.c
)

target_link_libraries(${DMOD_MODULE_NAME} example_lib_if )