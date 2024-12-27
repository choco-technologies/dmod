#
#  Paths
#
PROJECT_DIR=.
BUILD_DIR=$(PROJECT_DIR)/build

#
#   Definitions
#
DEFINITIONS=ON=1 \
	OFF=0 \
	DMOD_USE_STDLIB=ON \
	DMOD_USE_STDIO=ON \
	DMOD_USE_ASSERT=ON \
	DMOD_USE_PTHREAD=ON \
	DMOD_MAX_MODULES=30 \
	DMOD_MAX_REQUIRED_MODULES=10 \
	DMOD_MODE="DMOD_SYSTEM" \
	DMOD_SYSTEM_VERSION_MAJOR=0 \
	DMOD_SYSTEM_VERSION_MINOR=1 \
	DMOD_BUILD_TESTS=ON \
	DMOD_BUILD_EXAMPLES=ON \
	DMOD_DMF_DIR="dmf" \
	DMOD_REPO_DIR="" \
	DMOD_CPU_NAME=""

#
# 		Project configuration
#
include $(PROJECT_DIR)/cfg.mk

all: dmod_common dmod_module dmod_system

dmod_common:
	$(MAKE) $(DEFINITIONS) -C src/common 

dmod_module:
	$(MAKE) $(DEFINITIONS) -C src/module 

dmod_system:
	$(MAKE) $(DEFINITIONS) -C src/system 

clean:
	$(MAKE) -C src/common clean
	$(MAKE) -C src/module clean
	$(MAKE) -C src/system clean
	rm -rf $(BUILD_DIR)

.PHONY: all clean dmod_common dmod_module dmod_system