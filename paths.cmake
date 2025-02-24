#================================================================================================================================
# 	Script with paths of the project
#================================================================================================================================
if(NOT DEFINED DMOD_DIR)
	set(DMOD_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Disable echoing of commands
set(CMAKE_VERBOSE_MAKEFILE OFF)

# -----------------------------------------------------------------------------
# 	Output directories
# -----------------------------------------------------------------------------
if(NOT DEFINED DMOD_BUILD_DIR)
	if(NOT DEFINED DMOD_TOOLS_NAME)
		set(DMOD_BUILD_DIR ${DMOD_DIR}/build)
	else()
		set(DMOD_BUILD_DIR ${DMOD_DIR}/build/${DMOD_TOOLS_NAME})
	endif()
endif()
set(DMOD_OBJS_DIR ${DMOD_BUILD_DIR}/objs)
set(DMOD_LIBS_DIR ${DMOD_BUILD_DIR}/libs)
set(DMOD_TOOLS_BIN_DIR ${DMOD_BUILD_DIR}/bin/tools)
if(NOT DEFINED DMOD_DMF_DIR)
	set(DMOD_DMF_DIR ${DMOD_BUILD_DIR}/dmf)
endif()
if(NOT DEFINED DMOD_DMFC_DIR)
	set(DMOD_DMFC_DIR ${DMOD_BUILD_DIR}/dmfc)
endif()

# -----------------------------------------------------------------------------
#   Main project directories
# -----------------------------------------------------------------------------
set(DMOD_INC_DIR ${DMOD_DIR}/inc)
set(DMOD_SRC_DIR ${DMOD_DIR}/src)
set(DMOD_SCRIPTS_DIR ${DMOD_DIR}/scripts)
set(DMOD_EXAMPLES_DIR ${DMOD_DIR}/examples)
set(DMOD_TESTS_DIR ${DMOD_DIR}/tests)
set(DMOD_CONFIGS_DIR ${DMOD_DIR}/configs)

# -----------------------------------------------------------------------------
#   Makefile file names
# -----------------------------------------------------------------------------
set(DMOD_TOOLS_FILE_NAME tools-cfg.cmake)
set(DMOD_CFG_FILE_NAME dmod-cfg.cmake)
set(DMOD_DEFAULTS_FILE_NAME dmod-defaults.cmake)
set(DMOD_SUBDIRS_FILE_NAME subdirs.cmake)
set(DMOD_CONFIGURE_FILE_NAME configure_file.cmake)
set(DMOD_CONFIG_H_IN_FILE_NAME config.h.in)
set(DMOD_DMF_FILE_NAME dmf.cmake)
set(DMOD_DMF_LIB_FILE_NAME dmf-lib.cmake)
set(DMOD_DMF_APP_FILE_NAME dmf-app.cmake)
set(DMOD_MODULE_LD_FILE_NAME module.ld)
set(DMOD_API_HEADER_IN_FILE_NAME api.h.in)
set(DMOD_MODULE_HEADER_SOURCE_FILE_NAME dmod_header.c)
set(DMOD_INFO_MK_FILE_NAME dmod-info.cmake)
set(DMOD_COVERAGE_FILE_NAME coverage.html)
set(DMOD_COVERAGE_SUMMARY_FILE_NAME coverage.txt)

# -----------------------------------------------------------------------------
#   Makefile file paths
# -----------------------------------------------------------------------------
if(NOT DEFINED DMOD_TOOLS)
	if(NOT DEFINED DMOD_TOOLS_NAME)
		set(DMOD_TOOLS ${DMOD_DIR}/${DMOD_TOOLS_FILE_NAME})
	else()
		set(DMOD_TOOLS ${DMOD_CONFIGS_DIR}/${DMOD_TOOLS_NAME}/${DMOD_TOOLS_FILE_NAME})
	endif()
endif()
if(NOT DEFINED DMOD_CFG)
	set(DMOD_CFG ${DMOD_DIR}/${DMOD_CFG_FILE_NAME})
endif()
set(DMOD_SUBDIRS_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_SUBDIRS_FILE_NAME})
set(DMOD_SLIB_FILE_PATH ${DMOD_SCRIPTS_DIR}/staticlib.mk)
set(DMOD_CONFIGURE_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_CONFIGURE_FILE_NAME})
set(DMOD_CONFIG_H_IN_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_CONFIG_H_IN_FILE_NAME})
set(DMOD_DMF_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_DMF_FILE_NAME})
set(DMOD_DMF_LIB_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_DMF_LIB_FILE_NAME})
set(DMOD_DMF_APP_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_DMF_APP_FILE_NAME})
set(DMOD_MODULE_LD_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_MODULE_LD_FILE_NAME})
set(DMOD_API_HEADER_IN_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_API_HEADER_IN_FILE_NAME})
set(DMOD_MODULE_HEADER_SOURCE_FILE_PATH ${DMOD_SCRIPTS_DIR}/${DMOD_MODULE_HEADER_SOURCE_FILE_NAME})
set(DMOD_INFO_MK_FILE_PATH ${DMOD_DIR}/${DMOD_INFO_MK_FILE_NAME})
set(DMOD_COVERAGE_FILE_PATH ${DMOD_BUILD_DIR}/${DMOD_COVERAGE_FILE_NAME})
set(DMOD_COVERAGE_SUMMARY_FILE_PATH ${DMOD_BUILD_DIR}/${DMOD_COVERAGE_SUMMARY_FILE_NAME})
set(DMOD_DEFAULTS_FILE_PATH ${DMOD_DIR}/${DMOD_DEFAULTS_FILE_NAME})

# -----------------------------------------------------------------------------
#   Include the dmod configuration
# -----------------------------------------------------------------------------
include(${DMOD_CFG})
include(${DMOD_DEFAULTS_FILE_PATH})
include(${DMOD_INFO_MK_FILE_PATH})

# -----------------------------------------------------------------------------
#   List of extra definitions
# -----------------------------------------------------------------------------
if(DMOD_MODE STREQUAL "DMOD_SYSTEM")
	set(DMOD_SYSTEM ON)
	set(DMOD_MODULE OFF)
else()
	set(DMOD_SYSTEM OFF)
	set(DMOD_MODULE ON)
endif()