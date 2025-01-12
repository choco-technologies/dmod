# ##############################################################################
#
# 		Check if the cache has changed
#
# ##############################################################################

VARIABLES_LIST := $(filter DMOD% dmod%,$(.VARIABLES))
CACHE_CONTENT := $(foreach var,$(VARIABLES_LIST),$(var)=$($(var))\n)

# check if parent directory for tmp file exists
ifeq ($(wildcard $(DMOD_BUILD_DIR)),$(DMOD_BUILD_DIR))
  TMP_FILE := $(shell echo "$(CACHE_CONTENT)" > $(DMOD_TMP_CACHE_MK_FILE_PATH))
else
  TMP_FILE := 
endif

ifeq ($(wildcard $(DMOD_CACHE_MK_FILE_PATH)), $(DMOD_CACHE_MK_FILE_PATH))
  LAST_CACHE := $(shell cat $(DMOD_CACHE_MK_FILE_PATH))
  CHANGED := $(shell diff --color "$(DMOD_CACHE_MK_FILE_PATH)" "$(DMOD_TMP_CACHE_MK_FILE_PATH)" 2>&1 >> /dev/null; echo $$?)
else
  LAST_CACHE := 
  CHANGED    := 1
endif

ifeq ($(CHANGED),0)
  DMOD_CACHE_CHANGED = OFF
else
  DMOD_CACHE_CHANGED = ON
endif

#
#   Define the rule to update the cache
#
define update_cache
	@if [ "$(DMOD_UPDATE_CACHE)" = "ON" ]; then \
		echo "$(CACHE_CONTENT)" > $(DMOD_TMP_CACHE_MK_FILE_PATH); \
		echo "Comparing caches..."; \
		if [ -f "$(DMOD_CACHE_MK_FILE_PATH)" ]; then \
			if diff --color $(DMOD_CACHE_MK_FILE_PATH) $(DMOD_TMP_CACHE_MK_FILE_PATH); then \
				echo "No differences found. Cache update skipped."; \
			else \
				echo "Differences found. Updating cache... $(DMOD_CACHE_MK_FILE_PATH)"; \
				cp $(DMOD_TMP_CACHE_MK_FILE_PATH) $(DMOD_CACHE_MK_FILE_PATH); \
			fi; \
		else \
			echo "No existing cache file. Creating new cache... $(DMOD_CACHE_MK_FILE_PATH)"; \
			cp $(DMOD_TMP_CACHE_MK_FILE_PATH) $(DMOD_CACHE_MK_FILE_PATH); \
		fi; \
	else \
		echo "Skipping cache update..."; \
	fi
endef