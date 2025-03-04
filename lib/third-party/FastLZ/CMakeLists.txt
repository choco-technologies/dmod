set(MODULE_NAME dmod_fastlz)
add_library(${MODULE_NAME} STATIC
    fastlz.c
)

target_include_directories(${MODULE_NAME} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

# Link libraries if needed
target_link_libraries(${MODULE_NAME} 
    PRIVATE    
        
    )

target_compile_options(${MODULE_NAME} PRIVATE
    -Os
    -ffunction-sections
    -fdata-sections
)

create_library_makefile(${MODULE_NAME})