# ##############################################################################
#  Makefile for the dmod_version module
#
#     This Makefile is used to build the dmod_version module library
#
#   DONT EDIT THIS FILE - it is automatically generated. 
#   Edit the scripts/Makefile-lib.in file instead.
#       
# ##############################################################################
ifeq ($(DMOD_DIR),)
    DMOD_DIR = ../../../
endif

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_version.a
DMOD_SOURCES=dmod_version.c
DMOD_INC_DIRS = $(DMOD_DIR)/tools/lib/dmod_version\
		$(DMOD_DIR)/inc\
		$(DMOD_BUILD_DIR)\
		$(DMOD_DIR)/src/common\
		$(DMOD_DIR)/src/system
DMOD_LIBS = dmod_inc\
		dmod_common\
		dmod_system
DMOD_GEN_HEADERS_IN = 
DMOD_DEFINITIONS = 

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
# 	Including the template for the library
# -----------------------------------------------------------------------------
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/dmod_version
include $(DMOD_SLIB_FILE_PATH)
