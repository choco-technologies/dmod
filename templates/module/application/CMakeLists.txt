# Name of the module 
set(DMOD_MODULE_NAME        @MODULE_NAME@)

# Version (should be string in format "Major.Minor") 
set(DMOD_MODULE_VERSION     "0.1")

# Author (should be string)
set(DMOD_AUTHOR_NAME        @AUTHOR_NAME@)

# Stack size for the module (should be integer)
set(DMOD_STACK_SIZE         @STACK_SIZE@)

# Priority for the thread of the module (should be integer)
set(DMOD_PRIORITY           0)

# 
#   dmod_add_executable - create an executable module
#   it has the same signature as add_executable, 
#   and can be used in the same way after the creation
#   (for example, to link libraries)
#
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    # List of source files - can include C and C++ files
    main.c
)