ifeq ($(ON),)
	ON=1
endif
ifeq ($(OFF),)
	OFF=0
endif

# Use standard library
DMOD_USE_STDLIB=ON

# Use getenv function
DMOD_USE_GETENV=ON

# Use stdio library
DMOD_USE_STDIO=ON

# Use assert function
DMOD_USE_ASSERT=ON

# Use pthread library
DMOD_USE_PTHREAD=ON

# Use memory management functions
DMOD_USE_MMAN=ON

# Use aligned allocation
DMOD_USE_ALIGNED_ALLOC=ON

# Use aligned malloc mock if aligned allocation is not available
DMOD_USE_ALIGNED_MALLOC_MOCK=OFF

# Use realloc function
DMOD_USE_REALLOC=ON

# Use termios for terminal I/O control
DMOD_USE_TERMIOS=ON

# Use FastLZ compression library
DMOD_USE_FASTLZ=ON

# Maximum number of modules
DMOD_MAX_MODULES=30

# Maximum number of required modules
DMOD_MAX_REQUIRED_MODULES=10

# Mode of the system
DMOD_MODE="DMOD_SYSTEM"

# Major version of your system
DMOD_SYSTEM_VERSION_MAJOR=0

# Minor version of your system
DMOD_SYSTEM_VERSION_MINOR=1

# Build tests
DMOD_BUILD_TESTS=ON

# Build examples
DMOD_BUILD_EXAMPLES=ON

# Build tools
DMOD_BUILD_TOOLS=ON

# Build templates
DMOD_BUILD_TEMPLATES=ON

# Use exceptions
DMOD_USE_EXCEPTIONS=OFF

# Directory for DMFC files
DMOD_DMFC_DIR=${DMOD_BUILD_DIR}/dmfc

# Directory for DMF files
DMOD_DMF_DIR=${DMOD_BUILD_DIR}/dmf

# Path to the default repository inside the system
DMOD_REPO_DIR=${DMOD_DMF_DIR}

# Paths to the repositories inside the system in an array
DMOD_REPO_PATHS=${DMOD_DMF_DIR}${DMOD_ARRAY_SEP}${DMOD_DMFC_DIR}

# Name of the target cpu (if empty, the target is generic)
DMOD_CPU_NAME=""

# Name of the tools configuration
DMOD_TOOLS_NAME="arch/x86_64"

# Use debug mode
DMOD_DEBUG=ON