set(MODULE_NAME dmod_dmcomp)

# The decoder is always there; the encoder (it allocates ~330 kB and is ~10 kB of code) is for the host:
# a firmware that only loads the modules sets DMOD_USE_DMCOMP_ENCODER to OFF
add_library(${MODULE_NAME} STATIC
    dmcomp_inflate.c
    dmcomp_strerror.c
)
if(DMOD_USE_DMCOMP_ENCODER)
    target_sources(${MODULE_NAME} PRIVATE dmcomp_deflate.c)
else()
    target_compile_definitions(${MODULE_NAME} PUBLIC DMCOMP_NO_ENCODER)
endif()

target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(${MODULE_NAME} PRIVATE
    -Os
    -ffunction-sections
    -fdata-sections
)

create_library_makefile(${MODULE_NAME})
