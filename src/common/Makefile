# ##############################################################################
#  Makefile for the dmod_common module
#
#     This Makefile is used to build the dmod_common module library
#
#   DONT EDIT THIS FILE - it is automatically generated. 
#   Edit the scripts/Makefile-lib.in file instead.
#       
# ##############################################################################
DMOD_DIR = ../../

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_common.a
DMOD_SOURCES=dmod_common.c
DMOD_INC_DIRS = ../..//src/common\
		../..//inc\
		../..//build
DMOD_LIBS = dmod_inc
DMOD_GEN_HEADERS_IN = config.h.in

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
# 	Including the template for the library
# -----------------------------------------------------------------------------
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/dmod_common
include $(DMOD_SLIB_FILE_PATH)
