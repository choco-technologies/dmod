#================================================================================================================================
# 	Script with paths of the project
#================================================================================================================================
ifeq ($(DMOD_DIR),)
	DMOD_DIR=.
endif

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

# -----------------------------------------------------------------------------
#   Makefile file paths
# -----------------------------------------------------------------------------
ifeq ($(DMOD_TOOLS_FILE_PATH),)
	DMOD_TOOLS_FILE_PATH=$(DMOD_DIR)/$(DMOD_TOOLS_FILE_NAME)
endif
ifeq ($(DMOD_CFG_FILE_PATH),)
	DMOD_CFG_FILE_PATH=$(DMOD_DIR)/$(DMOD_CFG_FILE_NAME)
endif

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
dmod_create_out_dirs:
	@echo "Creating output directories..."
	@mkdir -p $(DMOD_BUILD_DIR)
	@mkdir -p $(DMOD_OBJS_DIR)
	@mkdir -p $(DMOD_DMF_DIR)
	@mkdir -p $(DMOD_LIBS_DIR)

dmod_clean_out_dirs:
	@echo "Cleaning output directories..."
	@rm -rf $(DMOD_BUILD_DIR)