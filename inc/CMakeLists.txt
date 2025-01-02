set(MODULE_NAME dmod_inc)

add_library(${MODULE_NAME} INTERFACE)

target_include_directories(${MODULE_NAME} 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DMOD_BUILD_DIR}
    )