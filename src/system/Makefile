# ##############################################################################
#  Makefile for the dmod_system module
#
#     This Makefile is used to build the dmod_system module library
#
#   DONT EDIT THIS FILE - it is automatically generated. 
#   Edit the scripts/Makefile-lib.in file instead.
#       
# ##############################################################################
DMOD_DIR = ../../

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_system.a
DMOD_SOURCES=dmod_system.c\
		dmod_if_file.c\
		dmod_if_mem.c\
		dmod_if_dbg.c\
		dmod_if_irq.c\
		dmod_if_events.c\
		dmod_if_rtos.c
DMOD_INC_DIRS = ../..//src/system\
		../..//inc\
		../..//build\
		../..//src/common
DMOD_LIBS = dmod_inc\
		dmod_common
DMOD_GEN_HEADERS_IN = 
DMOD_MODE = DMOD_SYSTEM
DMOD_SYSTEM = ON
DMOD_MODULE = OFF

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
# 	Including the template for the library
# -----------------------------------------------------------------------------
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/dmod_system
include $(DMOD_SLIB_FILE_PATH)
