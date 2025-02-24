# ===========================================================================
# 						Configuration options
# ===========================================================================

set(DMOD_DMF_DIR                "${CMAKE_CURRENT_BINARY_DIR}/dmf"                   CACHE STRING "Directory for DMF files")
set(DMOD_DMFC_DIR               "${CMAKE_CURRENT_BINARY_DIR}/dmfc"                  CACHE STRING "Directory for DMFC files")
set(DMOD_REPO_DIR               "${DMOD_DMF_DIR}" 				                    CACHE STRING "Directory for DMF files inside the system")
set(DMOD_REPO_PATHS             "${DMOD_DMF_DIR}${DMOD_ARRAY_SEP}${DMOD_DMFC_DIR}"  CACHE STRING "Paths to the repositories inside the system")    
set(DMOD_CPU_NAME			    "" 						  	                        CACHE STRING "Name of the target cpu, if empty, the target is generic")
set(DMOD_TOOLS_NAME			    "arch/x86_64" 					                    CACHE STRING "Name of the tools configuration")
set(DMOD_MIN_COVERAGE           40 							                        CACHE STRING "Minimum code coverage percentage")