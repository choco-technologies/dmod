# Name of the module 
set(DMOD_MODULE_NAME        @DMOD_MODULE_NAME@)

# Version (should be string in format "Major.Minor") 
set(DMOD_MODULE_VERSION     "0.1")

# Author (should be string)
set(DMOD_AUTHOR_NAME        @DMOD_AUTHOR_NAME@)

# Stack size for the module (should be integer)
set(DMOD_STACK_SIZE         @DMOD_STACK_SIZE@)

#
#   dmod_add_test - create a test module
#
#   The test runner main() is provided automatically.  Do NOT define main()
#   in your source files.  Register test steps with the DMOD_TEST_STEP()
#   macro from dmod_test.h.
#
dmod_add_test(${DMOD_MODULE_NAME} "0.1"
    # List of test source files - can include C and C++ files
    @DMOD_MODULE_NAME@_test.c
)
