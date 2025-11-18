# ##############################################################################
#  Makefile for the todmp executable
#
#     This Makefile is used to build the todmp executable
#
#   DON'T EDIT THIS FILE - it is automatically generated. 
#   Edit the scripts/Makefile-exe.in file instead.
#       
# ##############################################################################

# -----------------------------------------------------------------------------
# 	Paths configuration
# -----------------------------------------------------------------------------
DMOD_DIR=../../../
THIS_DIR=$(shell pwd)
TOOL_SRC_DIR=$(THIS_DIR)

# -----------------------------------------------------------------------------
# 	Project's definitions
# -----------------------------------------------------------------------------
PROJECT_NAME	= todmp
DMOD_SOURCES	= main.c
DMOD_INC_DIRS 	= $(DMOD_DIR)/inc\
		$(DMOD_BUILD_DIR)\
		$(DMOD_DIR)/tools/system/todmp
DMOD_LIBS 	  	= dmod_common\
		dmod_fastlz\
		dmod_system\
		pthread
DEFINITIONS 	= 

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_SYSTEM_TOOL_MK_FILE_PATH)
