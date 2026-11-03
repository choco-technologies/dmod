# -----------------------------------------------------------------------------
# 	Project properties
# -----------------------------------------------------------------------------
set(dmod_VERSION_MAJOR 1)
set(dmod_VERSION_MINOR 0)

set(DMOD_RELEASE_VERSION "${dmod_VERSION_MAJOR}.${dmod_VERSION_MINOR}" CACHE STRING
    "DMOD release version, e.g. 1.0.4 or 1.0.4-rc.1")
if(NOT DMOD_RELEASE_VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)(\\.(0|[1-9][0-9]*))?(-[0-9A-Za-z]+([.-][0-9A-Za-z]+)*)?$")
    message(FATAL_ERROR "DMOD_RELEASE_VERSION must be MAJOR.MINOR[.PATCH][-PRERELEASE]")
endif()
set(dmod_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(dmod_VERSION_MINOR "${CMAKE_MATCH_2}")
set(dmod_VERSION_PATCH "${CMAKE_MATCH_4}")
set(DMOD_VERSION_NUMERIC "${dmod_VERSION_MAJOR}.${dmod_VERSION_MINOR}")
if(dmod_VERSION_PATCH STREQUAL "")
    set(dmod_VERSION_PATCH 0)
else()
    string(APPEND DMOD_VERSION_NUMERIC ".${dmod_VERSION_PATCH}")
endif()

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE DMOD_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
