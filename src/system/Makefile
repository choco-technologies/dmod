# ##############################################################################
#  Makefile for the dmod_system module
#
#     This Makefile is used to build the dmod_system module library
#
#   DONT EDIT THIS FILE - it is automatically generated. Edit the Makefile.in
#   file instead.
#       
# ##############################################################################
DMOD_DIR = ../../

#
#  Name of the module
#
DMOD_LIB_NAME=libdmod_system.a

# -----------------------------------------------------------------------------
#  Main rule 
# -----------------------------------------------------------------------------
all: create_dirs $(DMOD_LIB_NAME)
	@echo "$(DMOD_LIB_NAME) is built"

# -----------------------------------------------------------------------------
# 	Initialization of paths
# -----------------------------------------------------------------------------
include $(DMOD_DIR)/paths.mk
include $(DMOD_CFG)
include $(DMOD_TOOLS)

# -----------------------------------------------------------------------------
# 	List of sources
# -----------------------------------------------------------------------------
DMOD_SOURCES=dmod_system.c\
		dmod_if_file.c\
		dmod_if_mem.c\
		dmod_if_dbg.c\
		dmod_if_irq.c\
		dmod_if_events.c\
		dmod_if_rtos.c

# -----------------------------------------------------------------------------
# 	List of objects
# -----------------------------------------------------------------------------
DMOD_OBJECTS = $(addprefix $(DMOD_OBJS_DIR)/dmod_system/, $(DMOD_SOURCES:.c=.o))

# -----------------------------------------------------------------------------
# 	List of include directories
# -----------------------------------------------------------------------------
DMOD_INC_DIRS = ../..//src/system\
		../..//inc\
		../..//build\
		../..//src/common
CFLAGS_INC = $(addprefix -I,$(DMOD_INC_DIRS))

# -----------------------------------------------------------------------------
# 	List of linked libraries
# -----------------------------------------------------------------------------
DMOD_LIBS = dmod_inc\
		dmod_common
CFLAGS_LIB = $(addprefix -L,$(DMOD_LIBS))

# -----------------------------------------------------------------------------
# 	List of compile definitions
# -----------------------------------------------------------------------------
DEFS = $(foreach v,$(filter VAR%,$(.VARIABLES)),-D$(v)=$($(v)))
DMOD_DEFINITIONS = DMOD_SYSTEM $(DEFS)
CFLAGS_DEF = $(addprefix -D,$(DMOD_DEFINITIONS))

# -----------------------------------------------------------------------------
# 	List of compiler flags
# -----------------------------------------------------------------------------
CFLAGS += $(CFLAGS_INC) $(CFLAGS_LIB) $(CFLAGS_DEF)

# -----------------------------------------------------------------------------
#   Rules
# -----------------------------------------------------------------------------
create_dirs: dmod_create_out_dirs
	@echo "Creating directories"
	@$(MKDIR) -p $(DMOD_OBJS_DIR)/dmod_system
	@$(MKDIR) -p $(DMOD_LIBS_DIR)

$(DMOD_LIB_NAME): $(DMOD_OBJECTS)
	@echo "Linking $(DMOD_LIB_NAME)"
	@$(AR) rcs $(DMOD_LIBS_DIR)/$(DMOD_LIB_NAME) $(DMOD_OBJECTS)

$(DMOD_OBJS_DIR)/dmod_system/%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -f $(DMOD_OBJECTS) $(DMOD_LIB_NAME)

.PHONY: all clean create_dirs
