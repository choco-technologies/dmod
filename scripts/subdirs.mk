# -----------------------------------------------------------------------------
# 	Main rule
# -----------------------------------------------------------------------------
all: create_dirs generate_headers build_subdirs
	@echo "All modules built..."

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_TOOLS)
include $(DMOD_CONFIGURE_FILE_PATH)

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
DMOD_GEN_HEADERS := $(addprefix $(DMOD_BUILD_DIR)/, $(DMOD_GEN_HEADERS_IN:.in=))

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
create_dirs:
	@echo "Creating output directories..."
	@$(MKDIR) -p $(DMOD_BUILD_DIR)
	@$(MKDIR) -p $(DMOD_OBJS_DIR)
	@$(MKDIR) -p $(DMOD_DMF_DIR)
	@$(MKDIR) -p $(DMOD_LIBS_DIR)

generate_headers: $(DMOD_GEN_HEADERS)
	@echo "All headers generated"
	@echo "List of generated headers: $(DMOD_GEN_HEADERS)"

build_subdirs: $(SUBDIRS)

$(DMOD_BUILD_DIR)/%.h: %.h.in
	@echo "Generating header $@"
	@$(call configure_file,$<,$@)

$(SUBDIRS):
	@echo "-----------------------------------------"
	@printf "Building \033[34;1m$@\033[0m\n"
	@$(MKDIR) -p $(DMOD_BUILD_DIR)/$@
	@$(MAKE) -C $@ DMOD_CFG=$(DMOD_CFG)

clean: 
	@echo "Cleaning all modules..."
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	@$(RM) -rf $(DMOD_BUILD_DIR)
	@echo "All modules cleaned..."

# Dont treat the following as files
.PHONY: create_dirs generate_headers build_subdirs $(SUBDIRS) clean