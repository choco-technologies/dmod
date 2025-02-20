#
# This is a makefile for the dmod loader example.
# It is used to build the dmod loader example.
#

# -----------------------------------------------------------------------------
# 	Paths configuration
# -----------------------------------------------------------------------------
DMOD_DIR=../..
MAIN_LD=main.ld
THIS_DIR=$(shell pwd)
DMOD_CFG=$(THIS_DIR)/dmod-cfg.mk

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_TOOLS)

# -----------------------------------------------------------------------------
# 	Project's definitions
# -----------------------------------------------------------------------------
PROJECT_NAME    = dmod_loader
OUTPUT_DIR		= $(DMOD_BUILD_DIR)/examples/system
DMOD_SOURCES	= main.c
DMOD_INC_DIRS 	= $(DMOD_INC_DIR) \
				  $(DMOD_BUILD_DIR) 
DMOD_LIBS 	  	= dmod_system\
				  dmod_common\
				  pthread
DEFINITIONS 	= 

ifeq ($(DMOD_USE_FASTLZ),ON)
	DMOD_LIBS += dmod_fastlz
endif

# -----------------------------------------------------------------------------
# 	List of objects
# -----------------------------------------------------------------------------
DMOD_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(DMOD_SOURCES:.c=.o))

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
	$(MAKE) -C $(DMOD_DIR) DMOD_CFG="$(DMOD_CFG)"
endif
$(PROJECT_NAME): $(DMOD_OBJECTS)
	$(CC) $(CFLAGS) -L $(DMOD_SCRIPTS_DIR) -T $(MAIN_LD) -o $(OUTPUT_DIR)/$(PROJECT_NAME) $(DMOD_OBJECTS) $(CFLAGS_LIB) $(CFLAGS_DEF)

$(OUTPUT_DIR)/%.o: %.c
	@$(MKDIR) -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rf $(OUTPUT_DIR)
	@$(RM) -rf $(OUTPUT_DIR)/$(PROJECT_NAME)

.PHONY: all clean