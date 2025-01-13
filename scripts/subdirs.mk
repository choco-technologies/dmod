# -----------------------------------------------------------------------------
# 	Main rule
# -----------------------------------------------------------------------------
all: create_dirs update_cache generate_headers build_subdirs
	@echo "All modules built..."

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_TOOLS)
include $(DMOD_CONFIGURE_FILE_PATH)

# -----------------------------------------------------------------------------
# 	List of parameters to pass to subdirectories
# -----------------------------------------------------------------------------
DMOD_PARAMS := DMOD_DIR=$(DMOD_DIR) DMOD_BUILD_DIR=$(DMOD_BUILD_DIR) DMOD_OBJS_DIR=$(DMOD_OBJS_DIR) DMOD_DMF_DIR=$(DMOD_DMF_DIR) DMOD_LIBS_DIR=$(DMOD_LIBS_DIR) DMOD_CFG=$(DMOD_CFG)

# -----------------------------------------------------------------------------
# 	Selection of subdirectories depending on the value of DMOD_MODE
# -----------------------------------------------------------------------------
ifeq ($(DMOD_SYSTEM),ON)
	SUBDIRS += $(SYSTEM_SUBDIRS)
endif
ifeq ($(DMOD_MODULE),ON)
	SUBDIRS += $(MODULE_SUBDIRS)
endif

# -----------------------------------------------------------------------------
#   List of headers to be generated
# -----------------------------------------------------------------------------
DMOD_GEN_HEADERS = $(foreach pair,$(DMOD_GEN_HEADERS_IN),$(word 2,$(subst =, ,$(pair))))

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
create_dirs:
	@echo "Creating output directories..."
	@$(MKDIR) -p $(DMOD_BUILD_DIR)
	@$(MKDIR) -p $(DMOD_OBJS_DIR)
	@$(MKDIR) -p $(DMOD_DMF_DIR)
	@$(MKDIR) -p $(DMOD_LIBS_DIR)

ifeq ($(shell expr $(MAKE_VERSION_MAJOR) \>= $(MIN_MAKE_VERSION_MAJOR) \& $(MAKE_VERSION_MINOR) \>= $(MIN_MAKE_VERSION_MINOR)),1)
update_cache:
	$(call update_cache)
	$(call touch_headers,$(DMOD_GEN_HEADERS_IN))

$(call generate_headers_rules,$(DMOD_GEN_HEADERS_IN))
else
update_cache:
	@echo "Make version is too old to support cache. Skipping cache update..."

$(DMOD_BUILD_DIR)/%.h: %.h.in
	@echo "Generating header $@"
	@$(call configure_file,$<,$@)
endif

generate_headers: $(DMOD_GEN_HEADERS)
	@echo "All headers generated"
	@echo "List of generated headers: $(DMOD_GEN_HEADERS)"

build_subdirs: $(SUBDIRS)

$(SUBDIRS):
	@echo "-----------------------------------------"
	@printf "Building \033[34;1m$@\033[0m\n"
	@$(MKDIR) -p $(DMOD_BUILD_DIR)/$@
	@$(MAKE) -C $@ $(DMOD_PARAMS)

clean: 
	@echo "Cleaning all modules..."
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $(DMOD_PARAMS) clean; \
	done
	@$(RM) -rf $(DMOD_BUILD_DIR)
	@echo "All modules cleaned..."

# Dont treat the following as files
.PHONY: create_dirs generate_headers build_subdirs $(SUBDIRS) clean update_cache