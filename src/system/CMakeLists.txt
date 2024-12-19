set(MODULE_NAME dmod_system)

add_library(${MODULE_NAME} STATIC
    dmod_system.c
)

target_include_directories(${MODULE_NAME} PUBLIC
    dmod_inc
)

# Link libraries if needed
# target_link_libraries(dmod_common some_other_library)