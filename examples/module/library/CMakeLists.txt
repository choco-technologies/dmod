set(DMOD_MODULE_NAME        example_lib)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        "Patryk Kubiak")

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    example_lib.c
)