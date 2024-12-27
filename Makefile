#
#  Paths
#
PROJECT_DIR=.
BUILD_DIR=$(PROJECT_DIR)/build

#
# 		Project configuration
#
include $(PROJECT_DIR)/cfg.mk

all: dmod_common dmod_module dmod_system

dmod_common:
	$(MAKE) -C src/common 

dmod_module:
	$(MAKE) -C src/module 

dmod_system:
	$(MAKE) -C src/system 

clean:
	$(MAKE) -C src/common clean
	$(MAKE) -C src/module clean
	$(MAKE) -C src/system clean
	rm -rf $(BUILD_DIR)

.PHONY: all clean dmod_common dmod_module dmod_system