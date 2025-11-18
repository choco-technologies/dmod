# ##############################################################################
#  Makefile for the dmod_dependencies module
#
#     This Makefile is used to build the dmod_dependencies module library
#       
# ##############################################################################
ifeq ($(DMOD_DIR),)
    DMOD_DIR = ../../../
endif

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_dependencies.a
DMOD_SOURCES=dmod_dependencies.c
DMOD_INC_DIRS = $(DMOD_DIR)/tools/lib/dmod_dependencies\
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
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/dmod_dependencies
include $(DMOD_SLIB_FILE_PATH)
