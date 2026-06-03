set(DMOD_MODULE_NAME        test_example)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "DMOD")

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    test_example_test.c
)
