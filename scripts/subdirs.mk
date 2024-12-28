# -----------------------------------------------------------------------------
# 	Main rule
# -----------------------------------------------------------------------------
all: create_dirs build_subdirs
	@echo "All modules built..."

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_CFG)
include $(DMOD_TOOLS)

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
create_dirs:
	@echo "Creating output directories..."
	@$(MKDIR) -p $(DMOD_BUILD_DIR)
	@$(MKDIR) -p $(DMOD_OBJS_DIR)
	@$(MKDIR) -p $(DMOD_DMF_DIR)
	@$(MKDIR) -p $(DMOD_LIBS_DIR)

build_subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MKDIR) -p $(DMOD_BUILD_DIR)/$@
	@echo "Building $@..."
	@$(MAKE) -C $@

clean: 
	@echo "Cleaning all modules..."
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	@$(RM) -rf $(DMOD_BUILD_DIR)
	@echo "All modules cleaned..."

# Dont treat the following as files
.PHONY: create_dirs build_subdirs $(SUBDIRS) clean