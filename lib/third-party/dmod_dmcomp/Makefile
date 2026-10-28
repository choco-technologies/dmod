# ##############################################################################
#  Makefile for the dmod_dmcomp module
#
#     This Makefile is used to build the dmod_dmcomp module library
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
DMOD_LIB_NAME=libdmod_dmcomp.a
DMOD_SOURCES=../dmcomp/src/dmcomp_inflate.c\
		../dmcomp/src/dmcomp_strerror.c\
		../dmcomp/src/dmcomp_deflate.c
DMOD_INC_DIRS = $(DMOD_DIR)/lib/third-party/dmod_dmcomp/../dmcomp/include\
		$(DMOD_DIR)/lib/third-party/dmod_dmcomp/../dmcomp/src
DMOD_LIBS = 
DMOD_GEN_HEADERS_IN = 
DMOD_DEFINITIONS = DMOD_CURRENT_ALLOCATOR="dmod_dmcomp"

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
# 	Including the template for the library
# -----------------------------------------------------------------------------
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/dmod_dmcomp
include $(DMOD_SLIB_FILE_PATH)
