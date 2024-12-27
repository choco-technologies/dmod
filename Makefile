#
#  Paths
#
PROJECT_DIR=.
BUILD_DIR=./build
CONFIG_IN=./config.h.in
CONFIG_OUT=./build/config.h

#
# 		Project configuration
#
include $(PROJECT_DIR)/dmod-cfg.mk

all: system

system: DMOD_MODE=DMOD_SYSTEM
system: DMOD_SYSTEM=ON
system: DMOD_MODULE=OFF
system: $(CONFIG_OUT) dmod_common dmod_system

module: DMOD_MODE=DMOD_MODULE
module: DMOD_SYSTEM=OFF
module: DMOD_MODULE=ON
module: $(CONFIG_OUT) dmod_common dmod_module

$(CONFIG_OUT): $(CONFIG_IN)
	@echo "Creating configuration file at $(CONFIG_OUT) (BUILD_DIR=$(BUILD_DIR))"
	@$(MKDIR) -p ./build
	@sed -e "s|@dmod_VERSION_MAJOR@|$(dmod_VERSION_MAJOR)|g" \
		-e "s|@dmod_VERSION_MINOR@|$(dmod_VERSION_MINOR)|g" \
		-e "s|@DMOD_MAX_MODULES@|$(DMOD_MAX_MODULES)|g" \
		-e "s|@DMOD_MAX_REQUIRED_MODULES@|$(DMOD_MAX_REQUIRED_MODULES)|g" \
		-e "s|@DMOD_USE_STDLIB@|$(DMOD_USE_STDLIB)|g" \
		-e "s|@DMOD_USE_STDIO@|$(DMOD_USE_STDIO)|g" \
		-e "s|@DMOD_USE_ASSERT@|$(DMOD_USE_ASSERT)|g" \
		-e "s|@DMOD_USE_PTHREAD@|$(DMOD_USE_PTHREAD)|g" \
		-e "s|@DMOD_MODE@|$(DMOD_MODE)|g" \
		-e "s|@DMOD_SYSTEM@|$(DMOD_SYSTEM)|g" \
		-e "s|@DMOD_MODULE@|$(DMOD_MODULE)|g" \
		-e "s|@DMOD_SYSTEM_VERSION_MAJOR@|$(DMOD_SYSTEM_VERSION_MAJOR)|g" \
		-e "s|@DMOD_SYSTEM_VERSION_MINOR@|$(DMOD_SYSTEM_VERSION_MINOR)|g" \
		-e "s|@DMOD_BUILD_TESTS@|$(DMOD_BUILD_TESTS)|g" \
		-e "s|@DMOD_BUILD_EXAMPLES@|$(DMOD_BUILD_EXAMPLES)|g" \
		-e "s|@DMOD_DMF_DIR@|$(DMOD_DMF_DIR)|g" \
		-e "s|@DMOD_REPO_DIR@|$(DMOD_REPO_DIR)|g" \
		-e "s|@DMOD_CPU_NAME@|$(DMOD_CPU_NAME)|g" \
		$(CONFIG_IN) > $(CONFIG_OUT)

dmod_common:
	@$(MAKE) -C src/common all

dmod_module:
	@$(MAKE) -C src/module all

dmod_system:
	@$(MAKE) -C src/system all

clean:
	@$(MAKE) -C src/common clean
	@$(MAKE) -C src/module clean
	@$(MAKE) -C src/system clean
	@$(RM) $(CONFIG_OUT)
	@rm -rf $(BUILD_DIR)

.PHONY: all clean dmod_common dmod_module dmod_system