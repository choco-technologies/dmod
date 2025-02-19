set(DMOD_MODULE_NAME        dmodex)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        "Patryk Kubiak")

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    dmodex.c
)