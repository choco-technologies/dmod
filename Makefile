#
# 	Main Makefile for the dmod project. You can include this file in your project Makefile
#
DMOD_DIR=$(shell pwd)

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
#   Subdirectories
# -----------------------------------------------------------------------------
DMOD_GEN_HEADERS_IN := config.h.in
SUBDIRS = src 
ifeq ($(DMOD_BUILD_EXAMPLES),ON)
	SUBDIRS += examples
endif

ifeq ($(DMOD_BUILD_TESTS),ON)
	SUBDIRS += tests
endif

# -----------------------------------------------------------------------------
# 	Include template makefile
# -----------------------------------------------------------------------------
include $(DMOD_SUBDIRS_FILE_PATH)