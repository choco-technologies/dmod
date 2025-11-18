# ##############################################################################
#  Makefile for the todmp executable
#
#     This Makefile is used to build the todmp executable
#
#   DON'T EDIT THIS FILE - it is automatically generated. 
#   Edit the scripts/Makefile-exe.in file instead.
#       
# ##############################################################################
ifeq ($(DMOD_DIR),)
    DMOD_DIR = ../../../
endif

#
#  Name of the executable
#
PROJECT_NAME=todmp
DMOD_SOURCES=main.c
DMOD_INC_DIRS = $(DMOD_DIR)/inc\
		$(DMOD_BUILD_DIR)\
		$(DMOD_DIR)/tools/system/todmp
DMOD_LIBS = dmod_common\
		dmod_fastlz\
		dmod_system\
		pthread
DEFINITIONS = 

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk

# -----------------------------------------------------------------------------
# 	Project's definitions
# -----------------------------------------------------------------------------
OUTPUT_DIR		= $(DMOD_BUILD_DIR)/tools/system/todmp
MAIN_LD			= $(DMOD_SCRIPTS_DIR)/main.ld
DMOD_EXE_SRC_PATH = $(DMOD_DIR)/tools/system/todmp
OBJ_OUTPUT_DIR	= $(DMOD_OBJS_DIR)/$(PROJECT_NAME)

# -----------------------------------------------------------------------------
# 	Initialization of tools configuration
# -----------------------------------------------------------------------------
include $(DMOD_TOOLS)

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
all: dmod $(PROJECT_NAME)

ifeq ($(DMOD_BUILD_EXAMPLES),ON)
dmod:
	@echo "Skipping dmod build"
else
dmod:
	$(MAKE) -C $(DMOD_DIR)
endif

$(PROJECT_NAME): $(DMOD_OBJECTS)
	@$(MKDIR) -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -L $(DMOD_SCRIPTS_DIR) -T $(MAIN_LD) -o $(OUTPUT_DIR)/$(PROJECT_NAME) $(DMOD_OBJECTS) $(CFLAGS_LIB) $(CFLAGS_DEF)

$(OBJ_OUTPUT_DIR)/%.o: $(DMOD_EXE_SRC_PATH)/%.c
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rf $(OUTPUT_DIR)/$(PROJECT_NAME)
	@$(RM) -rf $(OBJ_OUTPUT_DIR)

.PHONY: all clean dmod
