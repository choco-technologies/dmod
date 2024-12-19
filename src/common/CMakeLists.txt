set(MODULE_NAME dmod_common)

add_library(${MODULE_NAME} STATIC
    dmod_common.c
)

target_include_directories(${MODULE_NAME} 
    PUBLIC
)

# Link libraries if needed
target_link_libraries(dmod_common     
    PUBLIC    
	    dmod_inc
	)