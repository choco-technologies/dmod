#
#  Paths
#
PROJECT_DIR=.
BUILD_DIR=$(PROJECT_DIR)/build

#
# 		Project configuration
#
include $(PROJECT_DIR)/cfg.mk

#
#	 	Targets
# 
all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

