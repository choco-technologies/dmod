# ##############################################################################
#
#  	DMF library makefile
#
# ##############################################################################

# -----------------------------------------------------------------------------
# 	Default values
# -----------------------------------------------------------------------------
ifeq ($(DMOD_MODULE_NAME),)
	DMOD_MODULE_NAME=$(notdir $(shell pwd))
endif
ifeq ($(DMOD_MODULE_VERSION),)
	DMOD_MODULE_VERSION=0.1
endif
ifeq ($(DMOD_AUTHOR_NAME),)
	DMOD_AUTHOR_NAME=$(USER)
endif
ifeq ($(DMOD_STACK_SIZE),)
	DMOD_STACK_SIZE=1024
endif
ifeq ($(DMOD_PRIORITY),)
	DMOD_PRIORITY=1
endif
ifeq ($(DMOD_MANUAL_LOAD),)
	DMOD_MANUAL_LOAD=OFF
endif
ifeq ($(DMOD_COMPRESSION_METHOD),)
	DMOD_COMPRESSION_METHOD=fastlz
endif
ifeq ($(DMOD_USE_EXCEPTIONS),1)
	EXCEPTION_FLAGS :=
else
	EXCEPTION_FLAGS := -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables
endif
DMOD_MODULE_NAME_SNAKE_CASE := $(shell echo $(DMOD_MODULE_NAME) | sed 's/[A-Z]/_\l&/g')
DMOD_MODULE_DEFS_HEADER_FILE_NAME=$(DMOD_MODULE_NAME_SNAKE_CASE)_defs.h
DMOD_MODULE_DEFS_HEADER_FILE_PATH=$(DMOD_BUILD_DIR)/$(DMOD_MODULE_DEFS_HEADER_FILE_NAME)
DMOD_MODULE_HEADER_SOURCE_FILE_NAME=$(DMOD_MODULE_NAME_SNAKE_CASE)_header.c
DMOD_MODULE_HEADER_SOURCE_FILE_PATH=$(DMOD_BUILD_DIR)/$(DMOD_MODULE_HEADER_SOURCE_FILE_NAME)

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_TOOLS)
include $(DMOD_CONFIGURE_FILE_PATH)

# -----------------------------------------------------------------------------
# 	Configuration of paths
# -----------------------------------------------------------------------------
DMOD_LIB_OBJS_DIR = $(DMOD_OBJS_DIR)/$(DMOD_MODULE_NAME)
DMOD_MODULE_FILE_NAME = $(DMOD_MODULE_NAME).elf
DMOD_MODULE_FILE_PATH = $(DMOD_LIB_OBJS_DIR)/$(DMOD_MODULE_FILE_NAME)
DMOD_MODULE_DMF_FILE_NAME  = $(DMOD_MODULE_NAME).dmf
DMOD_MODULE_DMFC_FILE_NAME = $(DMOD_MODULE_NAME).dmfc
DMOD_MODULE_DMF_FILE_PATH  = $(DMOD_DMF_DIR)/$(DMOD_MODULE_DMF_FILE_NAME)
DMOD_MODULE_DMFC_FILE_PATH = $(DMOD_DMFC_DIR)/$(DMOD_MODULE_DMFC_FILE_NAME)
TODMFC 					 := $(shell command -v todmfc || command -v "$(DMOD_TOOLS_BIN_DIR)/todmfc")
TODMP 					 := $(shell command -v todmp || command -v "$(DMOD_TOOLS_BIN_DIR)/todmp")
TODMD 					 := $(shell command -v todmd || command -v "$(DMOD_TOOLS_BIN_DIR)/todmd")

# DMP package configuration
ifneq ($(DMOD_PACKAGE_NAME),)
	DMOD_DMP_DIR = $(DMOD_BUILD_DIR)/dmp
	DMOD_DMP_DMF_FILE_PATH = $(DMOD_DMP_DIR)/$(DMOD_PACKAGE_NAME)_dmf.dmp
	DMOD_DMP_DMFC_FILE_PATH = $(DMOD_DMP_DIR)/$(DMOD_PACKAGE_NAME)_dmfc.dmp
endif

# -----------------------------------------------------------------------------
# 	Add extra sources
# -----------------------------------------------------------------------------
DMOD_INC_DIRS       += $(DMOD_INC_DIR) $(DMOD_BUILD_DIR) $(DMOD_SCRIPTS_DIR)
DMOD_GEN_HEADERS_IN += $(DMOD_API_HEADER_IN_FILE_PATH)=$(DMOD_MODULE_DEFS_HEADER_FILE_PATH)	\
					   $(DMOD_MODULE_HEADER_SOURCE_IN_FILE_PATH)=$(DMOD_MODULE_HEADER_SOURCE_FILE_PATH)
DMOD_CSOURCES	    += $(DMOD_MODULE_HEADER_SOURCE_FILE_PATH)
DMOD_MAL_DEFS       += $(foreach impl,$(DMOD_MAL_IMPLS),DMOD_MAL_$(impl))
DMOD_DIF_DEFS       += $(foreach impl,$(DMOD_DIF_IMPLS),DMOD_DIF_$(impl))
DMOD_DEFINITIONS    += DMOD_${DMOD_MODULE_NAME} \
					   DMOD_MODULE_NAME=\"$(DMOD_MODULE_NAME)\" \
					   DMOD_MODULE_VERSION=\"$(DMOD_MODULE_VERSION)\" \
            		   DMOD_MODULE=1 \
            		   DMOD_SYSTEM=0 \
					   $(DMOD_MAL_DEFS) \
					   $(DMOD_DIF_DEFS)

# -----------------------------------------------------------------------------
# 	List of objects
# -----------------------------------------------------------------------------
DMOD_COBJECTS       = $(foreach src, $(DMOD_CSOURCES), $(src)=$(DMOD_LIB_OBJS_DIR)/$(basename $(notdir $(src))).o)
DMOD_CXXOBJECTS     = $(foreach src, $(DMOD_CXXSOURCES), $(src)=$(DMOD_LIB_OBJS_DIR)/$(basename $(notdir $(src))).o)
DMOD_OBJECTS        = $(foreach pair,$(DMOD_COBJECTS),$(word 2,$(subst =, ,$(pair)))) $(foreach pair,$(DMOD_CXXOBJECTS),$(word 2,$(subst =, ,$(pair))))
DMOD_GEN_HEADERS    = $(foreach pair,$(DMOD_GEN_HEADERS_IN),$(word 2,$(subst =, ,$(pair))))

# -----------------------------------------------------------------------------
# 	Preparation of compiler flags
# -----------------------------------------------------------------------------
C_OPT 				= -O2
CFLAGS_INC          = $(addprefix -I,$(DMOD_INC_DIRS))
CFLAGS_LIB          = $(addprefix -L,$(DMOD_LIBS))
CFLAGS_DEF          = $(addprefix -D,$(DMOD_DEFINITIONS))
OPTIMIZATION        = -O2
CFLAGS             += -fPIC -fPIE -ffunction-sections -fno-stack-protector -fno-stack-check -fno-split-stack -fno-builtin $(OPTIMIZATION) $(C_OPT) 
CFLAGS             += $(CFLAGS_INC) $(CFLAGS_LIB) $(CFLAGS_DEF)
CXXFLAGS           += $(CFLAGS)
LFLAGS 			   += -L $(DMOD_SCRIPTS_DIR) -T $(DMOD_MODULE_LD_FILE_NAME) -pie -nostartfiles -nostdlib -Xlinker --discard-all -static -Wl,--gc-sections,--undefined=ModuleHeader
ifeq ($(DMOD_DEBUG),ON)
	CFLAGS += -g
endif

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
all: create_dirs update_cache generate_headers $(DMOD_MODULE_DMF_FILE_PATH)
	@echo "List of sources: $(DMOD_COBJECTS) $(DMOD_CXXOBJECTS)"
	@printf "==== \033[32;1m$(DMOD_MODULE_DMF_FILE_PATH) has been built\033[0m ===\n"

ifeq ($(DMOD_UPDATE_CACHE),ON)
update_cache:
	$(call update_cache)
	$(call touch_headers,$(DMOD_GEN_HEADERS_IN))
else
update_cache:
	@echo "Make version is too old to support cache. Skipping cache update..."
endif

create_dirs: 
	@echo "Creating output directories"
	@$(MKDIR) -p $(DMOD_BUILD_DIR)
	@$(MKDIR) -p $(DMOD_OBJS_DIR)
	@$(MKDIR) -p $(DMOD_DMF_DIR)
	@$(MKDIR) -p $(DMOD_DMFC_DIR)
	@$(MKDIR) -p $(DMOD_LIBS_DIR)
	@$(MKDIR) -p $(DMOD_LIB_OBJS_DIR)
ifneq ($(DMOD_PACKAGE_NAME),)
	@$(MKDIR) -p $(DMOD_DMP_DIR)
endif

ifeq ($(DMOD_CONFIGURE_FILE_RULES),ON)
$(call generate_headers_rules,$(DMOD_GEN_HEADERS_IN))
else
$(DMOD_BUILD_DIR)/%.h: %.h.in
	@echo "Your make version is too old ($(MAKE_VERSION)) to support rules for configure file. Using legacy rules..."
	@$(call configure_file) $< $@
endif

generate_headers: $(DMOD_GEN_HEADERS)
	@echo "All headers generated"
	@echo "List of generated headers: $(DMOD_GEN_HEADERS)"

$(DMOD_MODULE_DMF_FILE_PATH): $(DMOD_OBJECTS)
	@echo "Linking $(DMOD_LIB_NAME)"
	$(CC) -o $(DMOD_MODULE_FILE_PATH) $(LFLAGS) $(DMOD_OBJECTS)
	@$(OBJCOPY) $(DMOD_MODULE_FILE_PATH) -O binary $(DMOD_MODULE_DMF_FILE_PATH) 
ifneq ($(TODMFC),)
	@echo "Compression of $(DMOD_MODULE_DMF_FILE_PATH)"
	@$(TODMFC) $(DMOD_MODULE_DMF_FILE_PATH) $(DMOD_MODULE_DMFC_FILE_PATH) $(DMOD_COMPRESSION_METHOD)
else
	@echo "todmfc is not found. Skipping compression..."
endif
ifneq ($(TODMD),)
	@echo "Generating dependencies file for $(DMOD_MODULE_DMF_FILE_PATH)"
	@$(TODMD) $(DMOD_MODULE_DMF_FILE_PATH) $(DMOD_DMF_DIR)/$(DMOD_MODULE_NAME).dmd
else
	@echo "todmd is not found. Skipping dependencies file generation..."
endif
ifneq ($(DMOD_PACKAGE_NAME),)
ifneq ($(TODMP),)
	@echo "Creating DMP package: $(DMOD_PACKAGE_NAME)"
	@echo "  - DMF package: $(DMOD_DMP_DMF_FILE_PATH)"
	@$(TODMP) $(DMOD_PACKAGE_NAME) $(DMOD_DMF_DIR) $(DMOD_DMP_DMF_FILE_PATH) $(DMOD_MAIN_MODULE_NAME)
ifneq ($(TODMFC),)
	@echo "  - DMFC package: $(DMOD_DMP_DMFC_FILE_PATH)"
	@$(TODMP) $(DMOD_PACKAGE_NAME) $(DMOD_DMFC_DIR) $(DMOD_DMP_DMFC_FILE_PATH) $(DMOD_MAIN_MODULE_NAME)
endif
else
	@echo "todmp is not found. Skipping DMP package creation..."
endif
endif

$(call generate_cobjects_rule,$(DMOD_CSOURCES))
$(call generate_cxxobjects_rule,$(DMOD_CXXSOURCES))

$(DMOD_LIB_OBJS_DIR)/%.o: $(DMOD_SCRIPTS_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DMOD_LIB_OBJS_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DMOD_LIB_OBJS_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@$(RM) -f $(DMOD_OBJECTS) $(DMOD_LIB_NAME)

.PHONY: all clean create_dirs generate_headers update_cache
