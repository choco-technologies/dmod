#
# This is a makefile for the system tools
#
# NOTE: .dmd file support is only available when building with CMake.
#       When building with make, only directory-based packaging is supported.
#

# -----------------------------------------------------------------------------
# 	Paths configuration
# -----------------------------------------------------------------------------
DMOD_DIR=../../..
THIS_DIR=$(shell pwd)
TOOL_SRC_DIR=$(THIS_DIR)

# -----------------------------------------------------------------------------
# 	Project's definitions
# -----------------------------------------------------------------------------
PROJECT_NAME    = todmp
DMOD_SOURCES	= main.c
DMOD_INC_DIRS 	= 
DMOD_LIBS 	  	= 
DEFINITIONS 	= 

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_SYSTEM_TOOL_MK_FILE_PATH)
