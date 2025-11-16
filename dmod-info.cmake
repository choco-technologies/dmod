# -----------------------------------------------------------------------------
# 	Project properties
# -----------------------------------------------------------------------------
set(dmod_VERSION_MAJOR 0)
set(dmod_VERSION_MINOR 1)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE DMOD_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
