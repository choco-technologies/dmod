#
# 	Main Makefile for the dmod project. You can include this file in your project Makefile
#
ifeq ($(DMOD_DIR),)
	DMOD_DIR=$(shell pwd)
endif

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
#   Subdirectories
# -----------------------------------------------------------------------------
DMOD_GEN_HEADERS_IN := $(DMOD_CONFIG_H_IN_FILE_PATH)=$(DMOD_CONFIG_H_FILE_PATH)
DMOD_UPDATE_CACHE := ON
SUBDIRS = src lib
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