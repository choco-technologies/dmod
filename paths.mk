#================================================================================================================================
# 	Script with paths of the project
#================================================================================================================================
ifeq ($(DMOD_DIR),)
	DMOD_DIR=$(shell pwd)
endif

# Disable echoing of commands
MAKEFLAGS += --no-print-directory

# -----------------------------------------------------------------------------
#   Platform specific definitions
# -----------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
	DMOD_ARRAY_SEP=;
	DMOD_PATH_SEP=\\
else
	DMOD_ARRAY_SEP=:
	DMOD_PATH_SEP=/
endif

# -----------------------------------------------------------------------------
# 	Output directories
# -----------------------------------------------------------------------------
ifeq ($(DMOD_BUILD_DIR),)
	ifeq ($(DMOD_TOOLS_NAME),)
		DMOD_BUILD_DIR=$(DMOD_DIR)/build
	else
		DMOD_BUILD_DIR=$(DMOD_DIR)/build/$(DMOD_TOOLS_NAME)
	endif
endif
DMOD_OBJS_DIR=$(DMOD_BUILD_DIR)/objs/$(DMOD_MODE)
DMOD_LIBS_DIR=$(DMOD_BUILD_DIR)/libs/$(DMOD_MODE)
DMOD_TOOLS_BIN_DIR=$(DMOD_BUILD_DIR)/bin/tools/
ifeq ($(DMOD_DMF_DIR),)
	DMOD_DMF_DIR=$(DMOD_BUILD_DIR)/dmf
endif
ifeq ($(DMOD_DMFC_DIR),)
	DMOD_DMFC_DIR=$(DMOD_BUILD_DIR)/dmfc
endif

# -----------------------------------------------------------------------------
#   Main project directories
# -----------------------------------------------------------------------------
DMOD_INC_DIR=$(DMOD_DIR)/inc
DMOD_SRC_DIR=$(DMOD_DIR)/src
DMOD_SCRIPTS_DIR=$(DMOD_DIR)/scripts
DMOD_EXAMPLES_DIR=$(DMOD_DIR)/examples
DMOD_TESTS_DIR=$(DMOD_DIR)/tests
DMOD_TOOLS_DIR=$(DMOD_DIR)/tools
DMOD_TOOLS_LIB_DIR=$(DMOD_TOOLS_DIR)/lib
DMOD_TOOLS_LIB_MANIFEST_DIR=$(DMOD_TOOLS_LIB_DIR)/dmod_manifest
DMOD_CONFIGS_DIR=$(DMOD_DIR)/configs
DMOD_EXAMPLE_DIFS_DIR=$(DMOD_EXAMPLES_DIR)/dif/difs

# -----------------------------------------------------------------------------
#   Makefile file names
# -----------------------------------------------------------------------------
DMOD_TOOLS_FILE_NAME=tools-cfg.mk
DMOD_CFG_FILE_NAME=dmod-cfg.mk
DMOD_SUBDIRS_FILE_NAME=subdirs.mk
DMOD_CONFIGURE_FILE_NAME=configure_file.mk
DMOD_CONFIG_H_IN_FILE_NAME=dmod-config.h.in
DMOD_CONFIG_H_FILE_NAME=dmod-config.h
DMOD_DMF_FILE_NAME=dmf.mk
DMOD_DMF_LIB_FILE_NAME=dmf-lib.mk
DMOD_DMF_APP_FILE_NAME=dmf-app.mk
DMOD_MODULE_LD_FILE_NAME=module.ld
DMOD_API_HEADER_IN_FILE_NAME=api.h.in
DMOD_MODULE_HEADER_SOURCE_IN_FILE_NAME=dmod_header.c.in
DMOD_INFO_MK_FILE_NAME=dmod-info.mk
DMOD_CACHE_MK_FILE_NAME=dmod-cache.mk
DMOD_TMP_CACHE_MK_FILE_NAME=dmod-cache-tmp.mk
DMOD_CHECK_CACHE_MK_FILE_NAME=check_cache.mk
DMOD_DEFAULTS_FILE_NAME=dmod-defaults.mk
DMOD_SYSTEM_TOOL_MK_FILE_NAME=system-tool.mk

# -----------------------------------------------------------------------------
#   Makefile file paths
# -----------------------------------------------------------------------------
ifeq ($(DMOD_TOOLS),)
	ifeq ($(DMOD_TOOLS_NAME),)
		DMOD_TOOLS=$(DMOD_DIR)/$(DMOD_TOOLS_FILE_NAME)
	else
		DMOD_TOOLS=$(DMOD_CONFIGS_DIR)/$(DMOD_TOOLS_NAME)/$(DMOD_TOOLS_FILE_NAME)
	endif
endif
ifeq ($(DMOD_CFG),)
	DMOD_CFG=$(DMOD_DIR)/$(DMOD_CFG_FILE_NAME)
endif
DMOD_SUBDIRS_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_SUBDIRS_FILE_NAME)
DMOD_SLIB_FILE_PATH=$(DMOD_SCRIPTS_DIR)/staticlib.mk
DMOD_CONFIGURE_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_CONFIGURE_FILE_NAME)
DMOD_CONFIG_H_IN_FILE_PATH=$(DMOD_DIR)/$(DMOD_CONFIG_H_IN_FILE_NAME)
DMOD_CONFIG_H_FILE_PATH=$(DMOD_BUILD_DIR)/$(DMOD_CONFIG_H_FILE_NAME)
DMOD_DMF_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_DMF_FILE_NAME)
DMOD_DMF_LIB_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_DMF_LIB_FILE_NAME)
DMOD_DMF_APP_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_DMF_APP_FILE_NAME)
DMOD_MODULE_LD_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_MODULE_LD_FILE_NAME)
DMOD_API_HEADER_IN_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_API_HEADER_IN_FILE_NAME)
DMOD_MODULE_HEADER_SOURCE_IN_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_MODULE_HEADER_SOURCE_IN_FILE_NAME)
DMOD_INFO_MK_FILE_PATH=$(DMOD_DIR)/$(DMOD_INFO_MK_FILE_NAME)
DMOD_CACHE_MK_FILE_PATH=$(DMOD_BUILD_DIR)/$(DMOD_CACHE_MK_FILE_NAME)
DMOD_TMP_CACHE_MK_FILE_PATH=$(DMOD_BUILD_DIR)/$(DMOD_TMP_CACHE_MK_FILE_NAME)
DMOD_CHECK_CACHE_MK_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_CHECK_CACHE_MK_FILE_NAME)
DMOD_DEFAULTS_FILE_PATH=$(DMOD_DIR)/$(DMOD_DEFAULTS_FILE_NAME)
DMOD_SYSTEM_TOOL_MK_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_SYSTEM_TOOL_MK_FILE_NAME)

# -----------------------------------------------------------------------------
#   Include the dmod configuration
# -----------------------------------------------------------------------------
include $(DMOD_CFG)
include $(DMOD_INFO_MK_FILE_PATH)
include $(DMOD_DEFAULTS_FILE_PATH)
include $(DMOD_CHECK_CACHE_MK_FILE_PATH)

# -----------------------------------------------------------------------------
#   List of extra definitions
# -----------------------------------------------------------------------------
ifeq ($(DMOD_MODE),DMOD_SYSTEM)
	DMOD_SYSTEM=ON
	DMOD_MODULE=OFF
else
	DMOD_SYSTEM=OFF
	DMOD_MODULE=ON
endif