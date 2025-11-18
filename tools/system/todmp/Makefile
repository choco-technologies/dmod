#
# This is a makefile for the system tools
#

# -----------------------------------------------------------------------------
# 	Paths configuration
# -----------------------------------------------------------------------------
DMOD_DIR=../../..
THIS_DIR=$(shell pwd)
TOOL_SRC_DIR=$(THIS_DIR)

include $(DMOD_DIR)/paths.mk
# -----------------------------------------------------------------------------
# 	Project's definitions
# -----------------------------------------------------------------------------
PROJECT_NAME    = todmp
DMOD_SOURCES	= main.c
DMOD_INC_DIRS 	= $(DMOD_TOOLS_LIB_MANIFEST_DIR) $(DMOD_TOOLS_LIB_DEPENDENCIES_DIR)
DMOD_LIBS 	  	= dmod_dependencies
DEFINITIONS 	= 

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_SYSTEM_TOOL_MK_FILE_PATH)
