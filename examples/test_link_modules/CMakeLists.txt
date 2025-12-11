set(DMOD_MODULE_NAME        test_link_modules)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        "Test Author")
set(DMOD_STACK_SIZE         1024)
set(DMOD_PRIORITY           0)

dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)

# Test the dmod_link_modules function
# This will attempt to download headers for dmlink module
# Note: This is just a test - the module may not exist in the registry
dmod_link_modules(${DMOD_MODULE_NAME}
    dmlink@1.0
)
