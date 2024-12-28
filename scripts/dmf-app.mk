# 
# Makefile for DMF applications
#

MODULE_LD=module.ld
BUILD_DIR=$(PROJECT_DIR)/build
OUTPUT_DIR=$(BUILD_DIR)/objs/$(DMOD_MODULE_NAME)
DMOD_DMF_DIR=$(BUILD_DIR)/dmf
DMOD_INC_DIRS += $(PROJECT_DIR)/inc $(PROJECT_DIR)/build $(PROJECT_DIR)/src/common $(PROJECT_DIR)/src/module
DMOD_INC_DIRS_CFLAGS = $(addprefix -I, $(DMOD_INC_DIRS))
DMOD_LIBS_CFLAGS = $(addprefix -l, $(DMOD_LIBS))
DMOD_DEFINITIONS_CFLAGS = $(addprefix -D, $(DMOD_DEFINITIONS))

CFLAGS += -fPIC -fPIE -ffunction-sections -fdata-sections $(DMOD_INC_DIRS_CFLAGS)
LFLAGS += -L $(SCRIPTS_DIR) -T $(MODULE_LD) -pie -nostartfiles -nostdlib -Xlinker --discard-all -static
DMOD_COBJECTS = $(addprefix $(OUTPUT_DIR)/, $(DMOD_CSOURCES:.c=.o))
DMOD_CXXOBJECTS = $(addprefix $(OUTPUT_DIR)/, $(DMOD_CXXSOURCES:.cpp=.o))
DMOD_OBJECTS = $(DMOD_COBJECTS) $(DMOD_CXXOBJECTS)

all: $(DMOD_MODULE_NAME)

$(DMOD_MODULE_NAME): $(DMOD_OBJECTS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(DMOD_DMF_DIR)/$(DMOD_MODULE_NAME).dmf $(DMOD_OBJECTS) 

$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR)/%.o: %.cpp
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OUTPUT_DIR)
	@rm -rf $(DMOD_DMF_DIR)/$(DMOD_MODULE_NAME).dmf
