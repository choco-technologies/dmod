# #############################################################################
#
#   This is an example of a DMOD test module.
#
# #############################################################################
DMOD_DIR=../../..

# -----------------------------------------------------------------------------
#  Paths initialization
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
#   Module configuration
# -----------------------------------------------------------------------------

# The name of the module
DMOD_MODULE_NAME=@MODULE_NAME@

# The version of the module
DMOD_MODULE_VERSION=0.1

# The name of the author
DMOD_AUTHOR_NAME=John Doe

# The list of C sources (do NOT add main.c - the test runner provides main)
DMOD_CSOURCES=@MODULE_NAME@_test.c

# The list of C++ sources
DMOD_CXXSOURCES=

# The list of include directories
DMOD_INC_DIRS=

# The list of libraries to link
DMOD_LIBS=

# The list of definitions
DMOD_DEFINITIONS=

# -----------------------------------------------------------------------------
#   Include the dmod app makefile (test modules are Application-type)
# -----------------------------------------------------------------------------
include $(DMOD_DMF_APP_FILE_PATH)
