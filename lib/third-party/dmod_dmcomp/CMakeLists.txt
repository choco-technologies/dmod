set(MODULE_NAME dmod_dmcomp)

# The sources are the submodule choco-technologies/dmcomp (lib/third-party/dmcomp): this only builds them the way DMOD
# builds its libraries (and writes the Makefile of the library, which the Make based builds use)
set(DMCOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../dmcomp")
if(NOT EXISTS "${DMCOMP_DIR}/include/dmcomp.h")
    message(FATAL_ERROR "The dmcomp library is missing: ${DMCOMP_DIR} is empty. "
                        "It is a git submodule - run 'git submodule update --init --recursive'.")
endif()

# The decoder is always there; the encoder (it allocates ~330 kB with malloc and is ~10 kB of code) is for the host:
# DMOD_USE_DMCOMP_ENCODER is OFF in a configuration without the standard library, a firmware that only loads modules
add_library(${MODULE_NAME} STATIC
    ../dmcomp/src/dmcomp_inflate.c
    ../dmcomp/src/dmcomp_strerror.c
)
if(DMOD_USE_DMCOMP_ENCODER)
    target_sources(${MODULE_NAME} PRIVATE ../dmcomp/src/dmcomp_deflate.c)
else()
    target_compile_definitions(${MODULE_NAME} PUBLIC DMCOMP_NO_ENCODER)
endif()

target_include_directories(${MODULE_NAME}
    PUBLIC
        ${DMCOMP_DIR}/include
    PRIVATE
        ${DMCOMP_DIR}/src
)

target_compile_options(${MODULE_NAME} PRIVATE
    -Os
    -ffunction-sections
    -fdata-sections
)

create_library_makefile(${MODULE_NAME})
