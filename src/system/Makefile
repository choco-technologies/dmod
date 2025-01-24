# ##############################################################################
#  Makefile for the dmod_system module
#
#     This Makefile is used to build the dmod_system module library
#
#   DONT EDIT THIS FILE - it is automatically generated. 
#   Edit the scripts/Makefile-lib.in file instead.
#       
# ##############################################################################
ifeq ($(DMOD_DIR),)
    DMOD_DIR = ../../
endif

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_system.a
DMOD_SOURCES=private/dmod_vars.c\
		private/dmod_ctx.c\
		private/dmod_hlp.c\
		private/dmod_ldr.c\
		private/dmod_mgr.c\
		private/dmod_rmod.c\
		dmod_system.c\
		if/dmod_if_file.c\
		if/dmod_if_mem.c\
		if/dmod_if_dbg.c\
		if/dmod_if_irq.c\
		if/dmod_if_events.c\
		if/dmod_if_rtos.c\
		Dmod.cpp
DMOD_INC_DIRS = $(DMOD_DIR)/src/system\
		$(DMOD_DIR)/inc\
		$(DMOD_BUILD_DIR)\
		$(DMOD_DIR)/src/common
DMOD_LIBS = dmod_inc\
		dmod_common\
		pthread
DMOD_GEN_HEADERS_IN = 
DMOD_DEFINITIONS = 

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
# 	Including the template for the library
# -----------------------------------------------------------------------------
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/dmod_system
include $(DMOD_SLIB_FILE_PATH)
