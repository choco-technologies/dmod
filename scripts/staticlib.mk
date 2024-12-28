# ##############################################################################
#
#  	Static library makefile
#
# ##############################################################################

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_CFG)
include $(DMOD_TOOLS)
include $(DMOD_CONFIGURE_FILE_PATH)

# -----------------------------------------------------------------------------
# 	List of objects
# -----------------------------------------------------------------------------
DMOD_OBJECTS := $(addprefix $(DMOD_LIB_OBJS_DIR)/, $(DMOD_SOURCES:.c=.o))
DMOD_GEN_HEADERS_IN := $(wildcard ./*.h.in)
DMOD_GEN_HEADERS    := $(addprefix $(DMOD_BUILD_DIR)/, $(DMOD_GEN_HEADERS_IN:.in=))

# -----------------------------------------------------------------------------
# 	Preparation of compiler flags
# -----------------------------------------------------------------------------
CFLAGS_INC 			= $(addprefix -I,$(DMOD_INC_DIRS))
CFLAGS_LIB 			= $(addprefix -L,$(DMOD_LIBS))
DEFS 				= $(foreach v,$(filter VAR%,$(.VARIABLES)),-D$(v)=$($(v)))
DMOD_DEFINITIONS 	= $(DEFS)
CFLAGS_DEF 			= $(addprefix -D,$(DMOD_DEFINITIONS))
CFLAGS 			   += $(CFLAGS_INC) $(CFLAGS_LIB) $(CFLAGS_DEF)

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
all: create_dirs generate_headers $(DMOD_LIB_NAME)
	@echo "$(DMOD_LIB_NAME) is built"

create_dirs: 
	@echo "Creating output directories"
	@$(MKDIR) -p $(DMOD_BUILD_DIR)
	@$(MKDIR) -p $(DMOD_OBJS_DIR)
	@$(MKDIR) -p $(DMOD_DMF_DIR)
	@$(MKDIR) -p $(DMOD_LIBS_DIR)
	@$(MKDIR) -p $(DMOD_LIB_OBJS_DIR)

generate_headers: $(DMOD_GEN_HEADERS)
	@echo "All headers generated"
	@echo "List of generated headers: $(DMOD_GEN_HEADERS)"

$(DMOD_LIB_NAME): $(DMOD_OBJECTS)
	@echo "Linking $(DMOD_LIB_NAME)"
	@$(AR) rcs $(DMOD_LIBS_DIR)/$(DMOD_LIB_NAME) $(DMOD_OBJECTS)

$(DMOD_LIB_OBJS_DIR)/%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(DMOD_BUILD_DIR)/%.h: %.h.in
	@$(call configure_file) $< $@

clean:
	@$(RM) -f $(DMOD_OBJECTS) $(DMOD_LIB_NAME)

.PHONY: all clean create_dirs generate_headers
