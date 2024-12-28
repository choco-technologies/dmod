# -----------------------------------------------------------------------------
# 	Main rule
# -----------------------------------------------------------------------------
all: create_dirs generate_headers build_subdirs
	@echo "All modules built..."

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_CFG)
include $(DMOD_TOOLS)
include $(DMOD_CONFIGURE_FILE_PATH)

# -----------------------------------------------------------------------------
#   List of headers to be generated
# -----------------------------------------------------------------------------
DMOD_GEN_HEADERS_IN := $(wildcard *.h.in)
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
	@$(call configure_file) $< $@

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
.PHONY: create_dirs generate_headers build_subdirs $(SUBDIRS) clean