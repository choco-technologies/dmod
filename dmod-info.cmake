# -----------------------------------------------------------------------------
# 	Project properties
# -----------------------------------------------------------------------------
set(dmod_VERSION_MAJOR 1)
set(dmod_VERSION_MINOR 0)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE DMOD_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
