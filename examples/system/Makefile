#
# This is a makefile for the dmod loader example.
# It is used to build the dmod loader example.
#

# ##############################################################################
#  Project paths
# ##############################################################################
DMOD_PROJECT_DIR=../..
SCRIPTS_DIR=$(DMOD_PROJECT_DIR)/scripts
MAIN_LD=main.ld
LIBS_DIR=$(DMOD_PROJECT_DIR)/build/libs
OUTPUT_DIR=$(DMOD_PROJECT_DIR)/build/examples/system

include $(DMOD_PROJECT_DIR)/dmod-cfg.mk

# ##############################################################################
#  Project configuration
# ##############################################################################
PROJECT_NAME=dmod_loader
DMOD_SOURCES=main.c

# ##############################################################################
#  List of object files
# ##############################################################################
DMOD_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(DMOD_SOURCES:.c=.o))

# ##############################################################################
#  Include directories
# ##############################################################################
DMOD_INC_DIRS = $(DMOD_PROJECT_DIR)/inc \
		$(DMOD_PROJECT_DIR)/build \
		$(DMOD_PROJECT_DIR)/src/common \
		$(DMOD_PROJECT_DIR)/src/system 

CFLAGS_INC = $(addprefix -I,$(DMOD_INC_DIRS))

# ##############################################################################
#  Linked libraries
# ##############################################################################
DMOD_LIBS = dmod_system dmod_common 

CFLAGS_LIB = $(addprefix -l,$(DMOD_LIBS)) -L $(LIBS_DIR)

# ##############################################################################
#  Definitions
# ##############################################################################
DEFS = $(foreach v,$(filter VAR%,$(.VARIABLES)),-D$(v)=$($(v)))
DMOD_DEFINITIONS =  $(DEFS)
CFLAGS_DEF = $(addprefix -D,$(DMOD_DEFINITIONS))

# ##############################################################################
#  Configuration of C flags
# ##############################################################################
CFLAGS+= $(CFLAGS_INC) $(CFLAGS_LIB) $(CFLAGS_DEF)

all: $(PROJECT_NAME)

$(PROJECT_NAME): $(DMOD_OBJECTS)
	$(CC) $(CFLAGS) -L $(SCRIPTS_DIR) -T $(MAIN_LD) -o $(OUTPUT_DIR)/$(PROJECT_NAME) $(DMOD_OBJECTS) $(CFLAGS_LIB) $(CFLAGS_DEF)

$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OUTPUT_DIR)
	@rm -rf $(OUTPUT_DIR)/$(PROJECT_NAME)
