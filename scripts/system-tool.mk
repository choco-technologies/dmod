#
# This is a makefile for the system tools 
# It is used to build the dmod loader example.
#

# -----------------------------------------------------------------------------
# 	Paths configuration
# -----------------------------------------------------------------------------
MAIN_LD			= $(DMOD_SCRIPTS_DIR)/main.ld
DMOD_INC_DIRS += $(DMOD_INC_DIR) \
				 $(DMOD_BUILD_DIR) 
DMOD_LIBS     += dmod_system\
				 dmod_common\
				 pthread
DMOD_CFG	   = $(TOOL_SRC_DIR)/dmod-cfg.mk

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_TOOLS)

# -----------------------------------------------------------------------------
# 	Project's definitions
# -----------------------------------------------------------------------------
OUTPUT_DIR		= $(DMOD_TOOLS_BIN_DIR)
OBJ_OUTPUT_DIR	= $(DMOD_OBJS_DIR)/$(PROJECT_NAME)

ifeq ($(DMOD_USE_FASTLZ),ON)
	DMOD_LIBS += dmod_fastlz
endif

# -----------------------------------------------------------------------------
# 	List of objects
# -----------------------------------------------------------------------------
DMOD_OBJECTS = $(addprefix $(OBJ_OUTPUT_DIR)/, $(DMOD_SOURCES:.c=.o))

# -----------------------------------------------------------------------------
# 	Preparation of C flags
# -----------------------------------------------------------------------------
CFLAGS_INC  = $(addprefix -I,$(DMOD_INC_DIRS))
CFLAGS_LIB 	= $(addprefix -l,$(DMOD_LIBS)) -L $(DMOD_LIBS_DIR)
CFLAGS_DEF  = $(addprefix -D,$(DEFINITIONS))
CFLAGS     += $(CFLAGS_INC) $(CFLAGS_LIB) $(CFLAGS_DEF)

# -----------------------------------------------------------------------------
# 	Build rules
# -----------------------------------------------------------------------------
all: check_params dmod $(PROJECT_NAME)

check_params:
ifeq ($(DMOD_DIR),)
	$(error DMOD_DIR is not defined)
endif
ifeq ($(PROJECT_NAME),)
	$(error PROJECT_NAME is not defined)
endif
ifeq ($(DMOD_SOURCES),)
	$(error DMOD_SOURCES is not defined)
endif
ifeq ($(DMOD_INC_DIRS),)
	$(warning DMOD_INC_DIRS is not defined)
endif
ifeq ($(TOOL_SRC_DIR),)
	$(error TOOL_SRC_DIR is not defined)
endif

ifeq ($(DMOD_BUILD_EXAMPLES),ON)
dmod:
	@echo "Skipping dmod build"
else
dmod:
	$(MAKE) -C $(DMOD_DIR) DMOD_CFG="$(DMOD_CFG)"
endif
$(PROJECT_NAME): $(DMOD_OBJECTS)
	$(CC) $(CFLAGS) -L $(DMOD_SCRIPTS_DIR) -T $(MAIN_LD) -o $(OUTPUT_DIR)/$(PROJECT_NAME) $(DMOD_OBJECTS) $(CFLAGS_LIB) $(CFLAGS_DEF)

$(OBJ_OUTPUT_DIR)/%.o: %.c
	@$(MKDIR) -p $(OUTPUT_DIR)
	@$(MKDIR) -p $(OBJ_OUTPUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rf $(OUTPUT_DIR)
	@$(RM) -rf $(OUTPUT_DIR)/$(PROJECT_NAME)
	@$(RM) -rf $(OBJ_OUTPUT_DIR)

.PHONY: all clean