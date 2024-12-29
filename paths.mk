#================================================================================================================================
# 	Script with paths of the project
#================================================================================================================================
ifeq ($(DMOD_DIR),)
	DMOD_DIR=.
endif

# Disable echoing of commands
MAKEFLAGS += --no-print-directory

# -----------------------------------------------------------------------------
# 	Output directories
# -----------------------------------------------------------------------------
DMOD_BUILD_DIR=$(DMOD_DIR)/build
DMOD_OBJS_DIR=$(DMOD_BUILD_DIR)/objs
DMOD_LIBS_DIR=$(DMOD_BUILD_DIR)/libs
ifeq ($(DMOD_DMF_DIR),)
	DMOD_DMF_DIR=$(DMOD_BUILD_DIR)/dmf
endif

# -----------------------------------------------------------------------------
#   Main project directories
# -----------------------------------------------------------------------------
DMOD_INC_DIR=$(DMOD_DIR)/inc
DMOD_SRC_DIR=$(DMOD_DIR)/src
DMOD_SCRIPTS_DIR=$(DMOD_DIR)/scripts
DMOD_EXAMPLES_DIR=$(DMOD_DIR)/examples
DMOD_TESTS_DIR=$(DMOD_DIR)/tests

# -----------------------------------------------------------------------------
#   Makefile file names
# -----------------------------------------------------------------------------
DMOD_TOOLS_FILE_NAME=tools-cfg.mk
DMOD_CFG_FILE_NAME=dmod-cfg.mk
DMOD_SUBDIRS_FILE_NAME=subdirs.mk
DMOD_CONFIGURE_FILE_NAME=configure_file.mk
DMOD_CONFIG_H_IN_FILE_NAME=config.h.in

# -----------------------------------------------------------------------------
#   Makefile file paths
# -----------------------------------------------------------------------------
ifeq ($(DMOD_TOOLS),)
	DMOD_TOOLS=$(DMOD_DIR)/$(DMOD_TOOLS_FILE_NAME)
endif
ifeq ($(DMOD_CFG),)
	DMOD_CFG=$(DMOD_DIR)/$(DMOD_CFG_FILE_NAME)
endif
DMOD_SUBDIRS_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_SUBDIRS_FILE_NAME)
DMOD_SLIB_FILE_PATH=$(DMOD_SCRIPTS_DIR)/staticlib.mk
DMOD_CONFIGURE_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_CONFIGURE_FILE_NAME)
DMOD_CONFIG_H_IN_FILE_PATH=$(DMOD_SCRIPTS_DIR)/$(DMOD_CONFIG_H_IN_FILE_NAME)

# -----------------------------------------------------------------------------
#   Include the dmod configuration
# -----------------------------------------------------------------------------
include $(DMOD_CFG)
include $(DMOD_TOOLS)

# -----------------------------------------------------------------------------
#   List of extra definitions
# -----------------------------------------------------------------------------
ifeq ($(DMOD_MODE),DMOD_SYSTEM)
	DMOD_SYSTEM=1
	DMOD_MODULE=0
else
	DMOD_SYSTEM=0
	DMOD_MODULE=1
endif