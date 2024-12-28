#
# 	Main Makefile for the dmod project. You can include this file in your project Makefile
#
ifeq ($(DMOD_DIR),)
	DMOD_DIR=.
endif

# -----------------------------------------------------------------------------
# 	Main rule
# -----------------------------------------------------------------------------
dmod_all: dmod_create_out_dirs dmod_common
	@echo "Building all modules..."

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_CFG_FILE_PATH)

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
dmod_common:
	@echo "Building common library..."
	@$(MAKE) -C $(DMOD_SRC_DIR)/common
