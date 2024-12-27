# ##############################################################################
#  Makefile for the dmod_system module
#
#     This Makefile is used to build the dmod_system module library
#
#   DONT EDIT THIS FILE - it is automatically generated. Edit the Makefile.in
#   file instead.
#       
# ##############################################################################

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_system.a
DMOD_PROJECT_DIR=../../
OUTPUT_DIR=../../build/src/system

#
#  Include project configuration
#
include $(DMOD_PROJECT_DIR)/cfg.mk

#
#  List of source files
#
DMOD_SOURCES=dmod_system.c\
		dmod_if_file.c\
		dmod_if_mem.c\
		dmod_if_dbg.c\
		dmod_if_irq.c\
		dmod_if_events.c\
		dmod_if_rtos.c

#
#  List of object files
#
DMOD_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(DMOD_SOURCES:.c=.o))

#
#  List of header directories
#
DMOD_INC_DIRS = ../..//src/system\
		../..//inc\
		../..//build\
		../..//src/common

#
#   Linked libraries
# 
DMOD_LIBS = dmod_inc\
		dmod_common

#
#   Definitions
#
DEFS = $(foreach v,$(filter VAR%,$(.VARIABLES)),-D$(v)=$($(v)))
DMOD_DEFINITIONS = DMOD_SYSTEM $(DEFS)

#
#  Add -I prefix to each include directory
#
CFLAGS_INC = $(addprefix -I,$(DMOD_INC_DIRS))

#
#  Add -L prefix to each library file
#
CFLAGS_LIB = $(addprefix -L,$(DMOD_LIBS))

#
#  Add -D prefix to each definition
#
CFLAGS_DEF = $(addprefix -D,$(DMOD_DEFINITIONS))

#
#  Compiler flags
#
CFLAGS += $(CFLAGS_INC) $(CFLAGS_LIB) $(CFLAGS_DEF)

#
#  List of targets
#
all: $(DMOD_LIB_NAME)

$(DMOD_LIB_NAME): $(DMOD_OBJECTS)
	$(AR) rcs $(OUTPUT_DIR)/$(DMOD_LIB_NAME) $(DMOD_OBJECTS)

$(OUTPUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DMOD_OBJECTS) $(DMOD_LIB_NAME)

.PHONY: all clean
