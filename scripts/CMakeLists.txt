if(NOT DEFINED DMOD_SCRIPTS_DIR)
    set(DMOD_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the scripts directory")
endif()

#
#   Tags every allocation a system-side target makes (Dmod_Malloc/Dmod_Free/...)
#   with its own identity, the same way DMOD_MODULE_NAME does for modules.
#
#   Usage:
#       dmod_set_current_allocator(targetName [allocatorName])
#
#   Defaults allocatorName to targetName itself, so calling this once per
#   target is enough to satisfy dmod_sal.h's requirement that DMOD_CURRENT_ALLOCATOR
#   be defined manually on the system side - no per-repo -D flag needed as long
#   as the target goes through this function (directly, or via
#   create_library_makefile()/dmod_add_tool(), which already call it).
#
function( dmod_set_current_allocator targetName )
    if(ARGC GREATER 1)
        set(ALLOCATOR_NAME ${ARGV1})
    else()
        set(ALLOCATOR_NAME ${targetName})
    endif()
    target_compile_definitions(${targetName} PRIVATE DMOD_CURRENT_ALLOCATOR="${ALLOCATOR_NAME}")
endfunction()

#
#   Creates makefile for the given library.
#
function( create_library_makefile targetName )
    # Check if DMOD_DIR is defined
    if(NOT DEFINED DMOD_DIR)
        message(FATAL_ERROR "DMOD_DIR is not defined. Please set DMOD_DIR variable before calling create_library_makefile function.")
    endif()

    # Every system-side library needs its own allocator identity (see dmod_sal.h) -
    # set it here, before COMPILE_DEFINITIONS is read below, so it also ends up in
    # the generated Makefile for non-CMake builds.
    dmod_set_current_allocator(${targetName})

    # Installed SDK sources are read-only and only used by CMake consumers.
    if(DEFINED DMOD_GENERATE_MAKEFILES AND NOT DMOD_GENERATE_MAKEFILES)
        return()
    endif()

    # Configure the Makefile
    if(DMOD_MODE STREQUAL "DMOD_SYSTEM")
        set(DMOD_SYSTEM ON)
        set(DMOD_MODULE OFF)
    elseif(DMOD_MODE STREQUAL "DMOD_MODULE")
        set(DMOD_SYSTEM OFF)
        set(DMOD_MODULE ON)
    else()
        set(DMOD_SYSTEM OFF)
        set(DMOD_MODULE OFF)
    endif()
    file(RELATIVE_PATH DMOD_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${DMOD_DIR}")
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_PROJECT_DIR "${DMOD_PROJECT_DIR}")
    
    set(DMOD_LIB_NAME ${targetName})
    file(RELATIVE_PATH DMOD_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    set(DMOD_OUTPUT_DIR ${DMOD_OUTPUT_DIR})
    
    set(DMOD_LIB_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_LIB_SRC_PATH "${DMOD_LIB_SRC_PATH}")
    
    set(DMOD_LIBS_DIR ${CMAKE_BINARY_DIR}/libs)
    string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" DMOD_LIBS_DIR "${DMOD_LIBS_DIR}")
    
    get_target_property(DMOD_SOURCES ${targetName} SOURCES)
    get_target_property(DMOD_INC_DIRS ${targetName} INCLUDE_DIRECTORIES)
    get_target_property(DMOD_LINKED_LIBRARIES ${targetName} LINK_LIBRARIES)
    get_target_property(DMOD_COMPILE_DEFINITIONS ${targetName} COMPILE_DEFINITIONS)
    # Check if the library has any linked libraries
    if(NOT DMOD_LINKED_LIBRARIES)
        set(DMOD_LINKED_LIBRARIES "")
    endif()

    foreach(LIB ${DMOD_LINKED_LIBRARIES})
        message(STATUS "LIB: ${LIB}")
        
        get_target_property(LIB_INC_DIRS ${LIB} INTERFACE_INCLUDE_DIRECTORIES)
        if(LIB_INC_DIRS)
            foreach(LIB_INC_DIR ${LIB_INC_DIRS})
                message(STATUS "LIB_INC_DIR: ${LIB_INC_DIR}")
                list(APPEND DMOD_INC_DIRS ${LIB_INC_DIR})
            endforeach()
        endif()
    endforeach()

    set(DMOD_DEFINITIONS "")
    if(DMOD_COMPILE_DEFINITIONS)
        foreach(DEFINITION ${DMOD_COMPILE_DEFINITIONS})
            message(STATUS "DEFINITION: ${DEFINITION}")
            list(APPEND DMOD_DEFINITIONS ${DEFINITION})
        endforeach()
    endif()

    set(DMOD_RELATIVE_HEADER_PATHS "")
    foreach(HEADER_PATH ${DMOD_INC_DIRS})
        # Check if path is absolute
        if(IS_ABSOLUTE "${HEADER_PATH}")
            # First try to replace with dmod variables
            string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" HEADER_PATH "${HEADER_PATH}")
            string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" HEADER_PATH "${HEADER_PATH}")
            
            # If still absolute, convert to relative path relative to CMAKE_CURRENT_SOURCE_DIR
            if(IS_ABSOLUTE "${HEADER_PATH}")
                file(RELATIVE_PATH RELATIVE_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HEADER_PATH}")
                set(HEADER_PATH "${RELATIVE_HEADER_PATH}")
            endif()
        endif()
        
        message(STATUS "HEADER_PATH: ${HEADER_PATH}")
        list(APPEND DMOD_RELATIVE_HEADER_PATHS "${HEADER_PATH}")
    endforeach()
    string(REPLACE ";" "\\\n\t\t" DMOD_SOURCES "${DMOD_SOURCES}")
    string(REPLACE ";" "\\\n\t\t" DMOD_INC_DIRS "${DMOD_RELATIVE_HEADER_PATHS}")
    string(REPLACE ";" "\\\n\t\t" DMOD_LIBS "${DMOD_LINKED_LIBRARIES}")
    string(REPLACE "Threads::Threads" "pthread" DMOD_LIBS "${DMOD_LIBS}")
    configure_file(${DMOD_DIR}/scripts/Makefile-lib.in ${CMAKE_CURRENT_SOURCE_DIR}/Makefile)
    
    # ===========================================================================
    # Memory Analysis for Static Libraries
    # ===========================================================================
    # Collect object files for memory analysis
    get_target_property(LIB_SOURCES ${targetName} SOURCES)
    set(LIB_OBJECT_FILES "")
    foreach(SOURCE_FILE ${LIB_SOURCES})
        get_filename_component(SOURCE_EXT "${SOURCE_FILE}" EXT)
        # Handle both .c and .cpp files
        if(SOURCE_EXT STREQUAL ".c")
            set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${targetName}.dir/${SOURCE_FILE}.o")
        elseif(SOURCE_EXT STREQUAL ".cpp")
            set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${targetName}.dir/${SOURCE_FILE}.o")
        else()
            continue()
        endif()
        list(APPEND LIB_OBJECT_FILES "${OBJ_FILE}")
    endforeach()
    
    # Convert list to string for passing to script
    string(REPLACE ";" "@@" LIB_OBJECT_FILES_STR "${LIB_OBJECT_FILES}")
    
    # Add custom command to show memory analysis after build
    add_custom_command(
        TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${LIB_OBJECT_FILES_STR}
            -DMODULE_NAME=${targetName}
            -DANALYSIS_TYPE=summary
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Analyzing memory usage for ${targetName}"
    )
    
    # Add custom target for detailed symbol analysis
    add_custom_target(${targetName}_memory_detailed
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${LIB_OBJECT_FILES_STR}
            -DMODULE_NAME=${targetName}
            -DANALYSIS_TYPE=detailed
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Detailed memory analysis for ${targetName}"
    )
endfunction()


function(to_snake_case INPUT_STRING OUTPUT_VARIABLE)
    # Replace hyphens with underscores
    string(REPLACE "-" "_" INTERMEDIATE_STRING "${INPUT_STRING}")
    
    # Replace spaces with underscores
    string(REPLACE " " "_" INTERMEDIATE_STRING "${INTERMEDIATE_STRING}")
    
    # Convert to lowercase
    string(TOLOWER "${INTERMEDIATE_STRING}" SNAKE_CASE_STRING)
    
    # Set the result to the output variable
    set(${OUTPUT_VARIABLE} "${SNAKE_CASE_STRING}" PARENT_SCOPE)
endfunction()

# ===========================================================================
# Assets conversion: views (.dmvs -> .dmv), images (-> .dmvi), fonts (-> .dmvf)
# ===========================================================================
#
# DMOD_ASSETS_PATHS - list of directories with the sources of a module's
# assets. When it is set, the first module created in a project converts
# them, with tools it downloads with dmf-get at configure time (dmf
# applications, run by dmod_loader on the build host) - only the tools for
# the kinds of files found:
#
#   *.dmvs                       todmv    -> <name>.dmv
#   images (DMOD_IMAGE_EXTENSIONS) todmvi -> <name>.dmvi, with the dmimg
#                                           decoder of each extension
#                                           (dmimg_png, dmimg_jpeg, ...)
#   fonts (DMOD_FONT_EXTENSIONS) todmvf   -> <section>.dmvf for each size in
#                                           its .ini
#
# Everything is placed in DMOD_VIEWS_DIR (default: <build>/views) - images
# and fonts next to the views, so a view finds them by their names. DMOD_VIEWS_DIR is
# then available to .dmr files as ${views_dir} (see mkdmrpkg --views-dir).
#
# An image's conversion is set by an optional <image>.ini next to it (e.g.
# logo.png.ini): keys before any section apply to every output; without
# sections there is one output, <name>.dmvi; each [section] is an output
# <section>.dmvi of its own:
#
#   format      = auto | rgb565 | rgb565a8 | argb8888 | i8 | a8 | a4
#   size        = WIDTHxHEIGHT (scaled down to fit), WIDTHx, xHEIGHT
#   compression = fastlz | ... | none
#   dither      = on | off
#   blur        = pixels - a Gaussian blur of that standard deviation, after scaling
#
# A font needs its <font>.ini (e.g. Roboto-Regular.ttf.ini), the same way:
# a font is made for one pixel size, so each output has one - each
# [section] a size, <section>.dmvf (without sections: <name>.dmvf). A font
# without an .ini is left out (with a warning):
#
#   size        = pixel size (em), 4 ... 127 - required
#   chars       = codepoint ranges, e.g. 0x20-0x7E,0x104 (default: printable
#                 ASCII, Latin-1, Latin Extended-A)
#   tracking    = letter spacing added to every advance, pixels, e.g. -2.4
#                 (CSS letter-spacing; default 0)
#
# Related variables:
#   DMOD_VIEWS_DIR          output directory of the converted assets
#   DMOD_VIEWS_TOOLS_NAME   tools name used to download the tools (host
#                           tools, default: arch/x86_64)
#   DMOD_LOADER             dmod_loader executable that runs the tools
#                           (default: searched in tools dir and PATH)
#   DMOD_IMAGE_EXTENSIONS   image file extensions (default: png jpg jpeg jpe
#                           jfif bmp qoi gif tif tiff svg)
#   DMOD_IMAGE_DECODERS     decoder modules to download (default:
#                           dmimg_<format> of each extension found)
#   DMOD_FONT_EXTENSIONS    font file extensions (default: ttf otf ttc)
#
# DMOD_FIXTURES_PATHS is the old name of DMOD_ASSETS_PATHS - still read, with
# a warning, when DMOD_ASSETS_PATHS is not set.
#
# Sets DMOD_VIEWS_ENABLED (parent scope) to TRUE when assets are converted.
#

# Download a tool (a dmf application and its dependencies) into dir; sets
# outVar to its .dmf, or to "" when it cannot be downloaded
function(_dmod_assets_tool toolName dir outVar)
    set(${outVar} "" PARENT_SCOPE)
    set(TOOL_DMF "${dir}/${toolName}.dmf")
    if(NOT EXISTS "${TOOL_DMF}")
        set(GET_OPTIONS -t ${DMOD_VIEWS_TOOLS_NAME} --type dmf -y)
        if(DEFINED DMOD_DMM_URL)
            list(APPEND GET_OPTIONS -m ${DMOD_DMM_URL})
        endif()
        message(STATUS "Downloading ${toolName} with dmf-get to ${dir}")
        file(MAKE_DIRECTORY "${dir}")
        execute_process(
            COMMAND ${DMF_GET} ${toolName} ${GET_OPTIONS} -o ${dir}
            RESULT_VARIABLE GET_RESULT
            OUTPUT_VARIABLE GET_OUTPUT
            ERROR_VARIABLE GET_ERROR
        )
        if(NOT GET_RESULT EQUAL 0 OR NOT EXISTS "${TOOL_DMF}")
            message(WARNING "Failed to download ${toolName}: ${GET_ERROR}${GET_OUTPUT}")
            return()
        endif()
    endif()
    set(${outVar} "${TOOL_DMF}" PARENT_SCOPE)
endfunction()

# The dmimg decoder module of an image extension (the aliases of dmimg)
function(_dmod_image_decoder extension outVar)
    string(TOLOWER "${extension}" EXT)
    if(EXT STREQUAL "jpg" OR EXT STREQUAL "jpe" OR EXT STREQUAL "jfif")
        set(EXT "jpeg")
    elseif(EXT STREQUAL "tif")
        set(EXT "tiff")
    endif()
    set(${outVar} "dmimg_${EXT}" PARENT_SCOPE)
endfunction()

# todmvi arguments of one ini key
function(_dmod_image_option iniPath key value outVar)
    set(ARGS "")
    if(key STREQUAL "format")
        set(ARGS -f ${value})
    elseif(key STREQUAL "size")
        set(ARGS -s ${value})
    elseif(key STREQUAL "compression")
        set(ARGS -c ${value})
    elseif(key STREQUAL "blur")
        if(NOT value MATCHES "^[0-9]+$")
            message(SEND_ERROR "${iniPath}: blur is a number of pixels, e.g. 8 (got '${value}')")
        endif()
        set(ARGS -b ${value})
    elseif(key STREQUAL "dither")
        string(TOLOWER "${value}" V)
        if(V MATCHES "^(off|no|false|0)$")
            set(ARGS --no-dither)
        elseif(NOT V MATCHES "^(on|yes|true|1)$")
            message(SEND_ERROR "${iniPath}: dither is on or off, not '${value}'")
        endif()
    else()
        message(SEND_ERROR "${iniPath}: unknown key '${key}' (format, size, compression, dither, blur)")
    endif()
    set(${outVar} "${ARGS}" PARENT_SCOPE)
endfunction()

# todmvf arguments of one font ini key (size is handled by _dmod_asset_outputs)
function(_dmod_font_option iniPath key value outVar)
    set(ARGS "")
    if(key STREQUAL "chars")
        set(ARGS -c ${value})
    elseif(key STREQUAL "tracking")
        if(NOT value MATCHES "^[-+]?[0-9]+(\\.[0-9]+)?$")
            message(SEND_ERROR "${iniPath}: tracking must be a number of pixels, e.g. -2.4 (got '${value}')")
        endif()
        set(ARGS -t ${value})
    else()
        message(SEND_ERROR "${iniPath}: unknown key '${key}' (size, chars, tracking)")
    endif()
    set(${outVar} "${ARGS}" PARENT_SCOPE)
endfunction()

# The outputs of an asset of a kind (image, font) - <prefix>_NAMES, and
# <prefix>_ARGS_<name>, for fonts also <prefix>_SIZE_<name> - from its .ini
# (see above). Without an .ini: one output with the defaults (an image), or
# none (a font - it has no size).
function(_dmod_asset_outputs kind source prefix)
    get_filename_component(NAME "${source}" NAME_WE)
    set(INI "${source}.ini")
    set(COMMON "")
    set(COMMON_SIZE "")
    set(SECTIONS "")
    set(${prefix}_NAMES "" PARENT_SCOPE)
    if(NOT EXISTS "${INI}")
        if(kind STREQUAL "font")
            message(WARNING "${source}: no ${INI} with its sizes - the font is left out")
            return()
        endif()
    else()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${INI}")
        file(STRINGS "${INI}" LINES)
        set(SECTION "")
        foreach(LINE ${LINES})
            string(REGEX REPLACE "[;#].*$" "" LINE "${LINE}")
            string(STRIP "${LINE}" LINE)
            if(LINE STREQUAL "")
                continue()
            elseif(LINE MATCHES "^\\[([A-Za-z0-9_.-]+)\\]$")
                set(SECTION "${CMAKE_MATCH_1}")
                list(APPEND SECTIONS "${SECTION}")
                set(ARGS_${SECTION} "")
                set(SIZE_${SECTION} "")
            elseif(LINE MATCHES "^([A-Za-z_]+)[ \t]*=[ \t]*(.*)$")
                set(KEY "${CMAKE_MATCH_1}")
                set(VALUE "${CMAKE_MATCH_2}")
                set(OPTION_ARGS "")
                if(kind STREQUAL "font" AND KEY STREQUAL "size")
                    if(NOT VALUE MATCHES "^[0-9]+$" OR VALUE LESS 4 OR VALUE GREATER 127)
                        message(SEND_ERROR "${INI}: size is 4 ... 127, not '${VALUE}'")
                    elseif(SECTION STREQUAL "")
                        set(COMMON_SIZE "${VALUE}")
                    else()
                        set(SIZE_${SECTION} "${VALUE}")
                    endif()
                elseif(kind STREQUAL "font")
                    _dmod_font_option("${INI}" "${KEY}" "${VALUE}" OPTION_ARGS)
                else()
                    _dmod_image_option("${INI}" "${KEY}" "${VALUE}" OPTION_ARGS)
                endif()
                if(SECTION STREQUAL "")
                    list(APPEND COMMON ${OPTION_ARGS})
                else()
                    list(APPEND ARGS_${SECTION} ${OPTION_ARGS})
                endif()
            else()
                message(SEND_ERROR "${INI}: cannot read '${LINE}'")
            endif()
        endforeach()
    endif()
    if(NOT SECTIONS)
        set(SECTIONS "${NAME}")
        set(ARGS_${NAME} "")
        set(SIZE_${NAME} "")
    endif()
    foreach(SECTION ${SECTIONS})
        set(${prefix}_ARGS_${SECTION} ${COMMON} ${ARGS_${SECTION}} PARENT_SCOPE)
        if(kind STREQUAL "font")
            set(SIZE "${SIZE_${SECTION}}")
            if(SIZE STREQUAL "")
                set(SIZE "${COMMON_SIZE}")
            endif()
            if(SIZE STREQUAL "")
                message(SEND_ERROR "${INI}: [${SECTION}] has no size")
            endif()
            set(${prefix}_SIZE_${SECTION} "${SIZE}" PARENT_SCOPE)
        endif()
    endforeach()
    set(${prefix}_NAMES ${SECTIONS} PARENT_SCOPE)
endfunction()

function(dmod_setup_assets moduleName)
    set(DMOD_VIEWS_ENABLED FALSE PARENT_SCOPE)
    if((NOT DEFINED DMOD_ASSETS_PATHS OR "${DMOD_ASSETS_PATHS}" STREQUAL "") AND
       DEFINED DMOD_FIXTURES_PATHS AND NOT "${DMOD_FIXTURES_PATHS}" STREQUAL "")
        get_property(FIXTURES_WARNED GLOBAL PROPERTY DMOD_FIXTURES_PATHS_WARNED)
        if(NOT FIXTURES_WARNED)
            message(WARNING "DMOD_FIXTURES_PATHS is deprecated - set DMOD_ASSETS_PATHS instead")
            set_property(GLOBAL PROPERTY DMOD_FIXTURES_PATHS_WARNED TRUE)
        endif()
        set(DMOD_ASSETS_PATHS "${DMOD_FIXTURES_PATHS}")
    endif()
    if(NOT DEFINED DMOD_ASSETS_PATHS OR "${DMOD_ASSETS_PATHS}" STREQUAL "")
        return()
    endif()

    if(NOT DEFINED DMOD_VIEWS_DIR OR "${DMOD_VIEWS_DIR}" STREQUAL "")
        set(DMOD_VIEWS_DIR "${CMAKE_BINARY_DIR}/views")
    endif()
    set(DMOD_VIEWS_DIR "${DMOD_VIEWS_DIR}" PARENT_SCOPE)
    if(NOT DEFINED DMOD_VIEWS_TOOLS_NAME OR "${DMOD_VIEWS_TOOLS_NAME}" STREQUAL "")
        set(DMOD_VIEWS_TOOLS_NAME "arch/x86_64")
    endif()
    if(NOT DEFINED DMOD_IMAGE_EXTENSIONS OR "${DMOD_IMAGE_EXTENSIONS}" STREQUAL "")
        set(DMOD_IMAGE_EXTENSIONS png jpg jpeg jpe jfif bmp qoi gif tif tiff svg)
    endif()
    if(NOT DEFINED DMOD_FONT_EXTENSIONS OR "${DMOD_FONT_EXTENSIONS}" STREQUAL "")
        set(DMOD_FONT_EXTENSIONS ttf otf ttc)
    endif()

    # The assets are converted once per project - later modules only depend on the target
    get_property(VIEWS_TARGET_READY GLOBAL PROPERTY DMOD_ASSETS_TARGET_READY)
    if(VIEWS_TARGET_READY)
        set(DMOD_VIEWS_ENABLED TRUE PARENT_SCOPE)
        add_dependencies(${moduleName} dmod_assets)
        return()
    endif()

    # Collect the sources
    set(VIEW_SOURCES "")
    set(IMAGE_SOURCES "")
    set(FONT_SOURCES "")
    foreach(ASSETS_PATH ${DMOD_ASSETS_PATHS})
        if(NOT IS_ABSOLUTE "${ASSETS_PATH}")
            set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/${ASSETS_PATH}")
        endif()
        if(NOT IS_DIRECTORY "${ASSETS_PATH}")
            message(WARNING "DMOD_ASSETS_PATHS: '${ASSETS_PATH}' is not a directory")
            continue()
        endif()
        file(GLOB FOUND_VIEWS "${ASSETS_PATH}/*.dmvs")
        list(APPEND VIEW_SOURCES ${FOUND_VIEWS})
        file(GLOB FOUND_FILES "${ASSETS_PATH}/*")
        foreach(FOUND ${FOUND_FILES})
            get_filename_component(EXT "${FOUND}" LAST_EXT)
            string(TOLOWER "${EXT}" EXT)
            string(REGEX REPLACE "^\\." "" EXT "${EXT}")
            if(IS_DIRECTORY "${FOUND}")
                continue()
            elseif(EXT IN_LIST DMOD_IMAGE_EXTENSIONS)
                list(APPEND IMAGE_SOURCES "${FOUND}")
            elseif(EXT IN_LIST DMOD_FONT_EXTENSIONS)
                list(APPEND FONT_SOURCES "${FOUND}")
            endif()
        endforeach()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ASSETS_PATH}")
    endforeach()
    if(NOT VIEW_SOURCES AND NOT IMAGE_SOURCES AND NOT FONT_SOURCES)
        message(STATUS "DMOD_ASSETS_PATHS: no assets found in ${DMOD_ASSETS_PATHS}")
        return()
    endif()

    find_program(DMF_GET dmf-get HINTS ${DMOD_TOOLS_BIN_DIR} PATHS ENV PATH)
    if(NOT DMF_GET)
        message(WARNING "dmf-get not found - assets from DMOD_ASSETS_PATHS will not be converted. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
        return()
    endif()
    if(NOT DMOD_LOADER)
        find_program(DMOD_LOADER dmod_loader
            HINTS ${DMOD_TOOLS_BIN_DIR} ${CMAKE_BINARY_DIR}/examples/system/dmod_loader
            PATHS ENV PATH)
    endif()
    if(NOT DMOD_LOADER)
        message(WARNING "dmod_loader not found (set DMOD_LOADER) - assets from DMOD_ASSETS_PATHS will not be converted")
        return()
    endif()

    file(MAKE_DIRECTORY "${DMOD_VIEWS_DIR}")
    set(OUTPUTS "")

    # Views
    if(VIEW_SOURCES)
        set(TODMV_DIR "${CMAKE_BINARY_DIR}/todmv")
        _dmod_assets_tool(todmv "${TODMV_DIR}" TODMV_DMF)
        if(NOT TODMV_DMF)
            message(WARNING "Views will not be converted")
            set(VIEW_SOURCES "")
        endif()
    endif()
    foreach(VIEW_SOURCE ${VIEW_SOURCES})
        get_filename_component(VIEW_NAME "${VIEW_SOURCE}" NAME_WE)
        set(VIEW_OUTPUT "${DMOD_VIEWS_DIR}/${VIEW_NAME}.dmv")
        add_custom_command(
            OUTPUT "${VIEW_OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E env DMOD_DMF_DIR=${TODMV_DIR}
                    ${DMOD_LOADER} ${TODMV_DMF} --args -o ${VIEW_OUTPUT} ${VIEW_SOURCE}
            DEPENDS "${VIEW_SOURCE}"
            COMMENT "Converting view ${VIEW_NAME}.dmvs to ${VIEW_NAME}.dmv"
            VERBATIM
        )
        list(APPEND OUTPUTS "${VIEW_OUTPUT}")
    endforeach()

    # Images: todmvi, and the decoders it needs
    if(IMAGE_SOURCES)
        set(TODMVI_DIR "${CMAKE_BINARY_DIR}/todmvi")
        _dmod_assets_tool(todmvi "${TODMVI_DIR}" TODMVI_DMF)
        set(DECODERS "${DMOD_IMAGE_DECODERS}")
        if(NOT DECODERS)
            foreach(IMAGE_SOURCE ${IMAGE_SOURCES})
                get_filename_component(EXT "${IMAGE_SOURCE}" LAST_EXT)
                string(REGEX REPLACE "^\\." "" EXT "${EXT}")
                _dmod_image_decoder("${EXT}" DECODER)
                list(APPEND DECODERS ${DECODER})
            endforeach()
            list(REMOVE_DUPLICATES DECODERS)
        endif()
        foreach(DECODER ${DECODERS})
            if(TODMVI_DMF)
                _dmod_assets_tool(${DECODER} "${TODMVI_DIR}" DECODER_DMF)
                if(NOT DECODER_DMF)
                    message(WARNING "No ${DECODER} - images it decodes will not be converted")
                endif()
            endif()
        endforeach()
        if(NOT TODMVI_DMF)
            message(WARNING "Images will not be converted")
            set(IMAGE_SOURCES "")
        endif()
    endif()
    foreach(IMAGE_SOURCE ${IMAGE_SOURCES})
        get_filename_component(IMAGE_FILE "${IMAGE_SOURCE}" NAME)
        set(IMAGE_DEPENDS "${IMAGE_SOURCE}")
        if(EXISTS "${IMAGE_SOURCE}.ini")
            list(APPEND IMAGE_DEPENDS "${IMAGE_SOURCE}.ini")
        endif()
        _dmod_asset_outputs(image "${IMAGE_SOURCE}" IMAGE)
        foreach(OUTPUT_NAME ${IMAGE_NAMES})
            set(IMAGE_OUTPUT "${DMOD_VIEWS_DIR}/${OUTPUT_NAME}.dmvi")
            add_custom_command(
                OUTPUT "${IMAGE_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E env DMOD_DMF_DIR=${TODMVI_DIR}
                        ${DMOD_LOADER} ${TODMVI_DMF} --args -q ${IMAGE_ARGS_${OUTPUT_NAME}} -o ${IMAGE_OUTPUT} ${IMAGE_SOURCE}
                DEPENDS ${IMAGE_DEPENDS}
                COMMENT "Converting image ${IMAGE_FILE} to ${OUTPUT_NAME}.dmvi"
                VERBATIM
            )
            list(APPEND OUTPUTS "${IMAGE_OUTPUT}")
        endforeach()
    endforeach()

    # Fonts: todmvf
    if(FONT_SOURCES)
        set(TODMVF_DIR "${CMAKE_BINARY_DIR}/todmvf")
        _dmod_assets_tool(todmvf "${TODMVF_DIR}" TODMVF_DMF)
        if(NOT TODMVF_DMF)
            message(WARNING "Fonts will not be converted")
            set(FONT_SOURCES "")
        endif()
    endif()
    foreach(FONT_SOURCE ${FONT_SOURCES})
        get_filename_component(FONT_FILE "${FONT_SOURCE}" NAME)
        _dmod_asset_outputs(font "${FONT_SOURCE}" FONT)
        foreach(OUTPUT_NAME ${FONT_NAMES})
            set(FONT_OUTPUT "${DMOD_VIEWS_DIR}/${OUTPUT_NAME}.dmvf")
            add_custom_command(
                OUTPUT "${FONT_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E env DMOD_DMF_DIR=${TODMVF_DIR}
                        ${DMOD_LOADER} ${TODMVF_DMF} --args -q ${FONT_ARGS_${OUTPUT_NAME}} -o ${FONT_OUTPUT}
                        ${FONT_SOURCE} ${FONT_SIZE_${OUTPUT_NAME}}
                DEPENDS "${FONT_SOURCE}" "${FONT_SOURCE}.ini"
                COMMENT "Converting font ${FONT_FILE} to ${OUTPUT_NAME}.dmvf"
                VERBATIM
            )
            list(APPEND OUTPUTS "${FONT_OUTPUT}")
        endforeach()
    endforeach()

    # Two sources of one output
    set(SEEN "")
    foreach(OUTPUT ${OUTPUTS})
        if(OUTPUT IN_LIST SEEN)
            message(SEND_ERROR "DMOD_ASSETS_PATHS: ${OUTPUT} is made from two sources - rename one (or its .ini section)")
        endif()
        list(APPEND SEEN "${OUTPUT}")
    endforeach()
    if(NOT OUTPUTS)
        return()
    endif()

    add_custom_target(dmod_assets ALL DEPENDS ${OUTPUTS})
    set_property(GLOBAL PROPERTY DMOD_ASSETS_TARGET_READY TRUE)
    add_dependencies(${moduleName} dmod_assets)
    message(STATUS "Assets: converting ${VIEW_SOURCES} ${IMAGE_SOURCES} ${FONT_SOURCES} to ${DMOD_VIEWS_DIR}")
    set(DMOD_VIEWS_ENABLED TRUE PARENT_SCOPE)
endfunction()

#
# Function to create a DMOD module
#
function(dmod_create_module moduleName version moduleType)    
    set(SOURCES ${ARGN})

    set(DMOD_MODULE_NAME ${moduleName})
    to_snake_case(${moduleName} DMOD_MODULE_NAME_SNAKE_CASE)
    set(DMOD_MODULE_VERSION ${version})
    if(NOT DEFINED DMOD_AUTHOR_NAME)
        set(DMOD_AUTHOR_NAME "Unknown")
    endif()
    if(NOT DEFINED DMOD_STACK_SIZE)
        set(DMOD_STACK_SIZE 1024)
    endif()
    if(NOT DEFINED DMOD_PRIORITY)
        set(DMOD_PRIORITY 1)
    endif()
    if(NOT DEFINED DMOD_MANUAL_LOAD)
        set(DMOD_MANUAL_LOAD OFF)
    endif()
    set(DMOD_MODULE_TYPE ${moduleType})

    set(DMOD_LIBRARY_MODULE 0)
    set(DMOD_APPLICATION_MODULE 0)

    if(moduleType STREQUAL "Library")
        set(DMOD_LIBRARY_MODULE 1)
    elseif(moduleType STREQUAL "Application")
        set(DMOD_APPLICATION_MODULE 1)
    endif()

    # check if todmfc tool is available
    find_program(TODMFC 
            todmfc
            HINTS ${DMOD_TOOLS_BIN_DIR}
            )
    if(NOT TODMFC)
        if(DEFINED DMOD_COMPRESSION_METHOD)
            message(WARNING "todmfc not found, compression method will be set to none")
        endif()
        set(DMOD_COMPRESSION_METHOD "none")
    else()
        message(STATUS "todmfc found: ${TODMFC}")
        if(NOT DEFINED DMOD_COMPRESSION_METHOD)
            set(DMOD_COMPRESSION_METHOD "fastlz")
        endif()
    endif()

    # check if todmd tool is available
    find_program(TODMD 
            todmd
            HINTS ${DMOD_TOOLS_BIN_DIR}
            )

    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE DMOD_MODULE_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    configure_file(${DMOD_VERSION_IN_FILE_PATH} ${DMOD_DMF_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_version.txt @ONLY)
    configure_file(${DMOD_SCRIPTS_DIR}/api.h.in ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_defs.h)
    configure_file(${DMOD_MODULE_HEADER_SOURCE_IN_FILE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_header.c)
    
    add_executable(${moduleName} 
        ${SOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_header.c
        )
    
    add_library(${moduleName}_if INTERFACE
        )
    
    # target_precompile_headers(${moduleName} 
    #     PRIVATE 
    #         ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_defs.h
    #     )

    foreach(MAL ${DMOD_MAL_IMPLS})
        target_compile_definitions(${moduleName} 
            PRIVATE
                DMOD_MAL_${MAL}
                )
    endforeach()

    foreach(DIF ${DMOD_DIF_IMPLS})
        target_compile_definitions(${moduleName} 
            PRIVATE
                DMOD_DIF_${DIF}
                )
    endforeach()

    target_compile_definitions(${moduleName} 
        PUBLIC
            DMOD_${moduleName}
            DMOD_MODE="DMOD_MODULE"
            DMOD_MODULE_NAME="${moduleName}"
            DMOD_MODULE_VERSION="${version}"
            DMOD_MODULE=1
            DMOD_LIBRARY_MODULE=${DMOD_LIBRARY_MODULE}
            DMOD_APPLICATION_MODULE=${DMOD_APPLICATION_MODULE}
            DMOD_SYSTEM=0
        )

    target_include_directories(${moduleName} 
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    target_include_directories(${moduleName}_if 
        INTERFACE
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    
    target_link_libraries(${moduleName} dmod ${moduleName}_if)

    # Link against libgcc explicitly since -nostdlib is used.
    # This provides ARM runtime helpers like __aeabi_uldivmod (64-bit division),
    # __aeabi_fadd, __aeabi_f2iz (float operations), etc.
    separate_arguments(LIBGCC_QUERY_FLAGS UNIX_COMMAND "${CMAKE_C_FLAGS}")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} ${LIBGCC_QUERY_FLAGS} -print-libgcc-file-name
        OUTPUT_VARIABLE LIBGCC_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(LIBGCC_PATH AND EXISTS "${LIBGCC_PATH}")
        target_link_libraries(${moduleName} ${LIBGCC_PATH})
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(OPTIMIZATION "-Og")
    else()
        set(OPTIMIZATION "-O2")
    endif()
    if(DMOD_USE_EXCEPTIONS)
        set(EXCEPTION_FLAGS "")
    else()
        set(EXCEPTION_FLAGS -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables)
    endif()

    target_compile_options(${moduleName} PRIVATE -fPIC -fPIE -ffunction-sections ${EXCEPTION_FLAGS} -fdata-sections ${OPTIMIZATION} -fno-stack-protector -fno-builtin )
    target_compile_options(${moduleName} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
    )
    target_link_options(${moduleName} PRIVATE -L ${DMOD_SCRIPTS_DIR} -pie -nostartfiles -nostdlib -Xlinker --discard-all -Wl,--gc-sections,--undefined=ModuleHeader )
    target_link_options(${moduleName} PRIVATE -T ${DMOD_SCRIPTS_DIR}/module.ld  -static)
    
    set(ELF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${moduleName}")
    set(DMF_OUTPUT "${DMOD_DMF_DIR}/${moduleName}.dmf")
    set(DMFC_OUTPUT "${DMOD_DMFC_DIR}/${moduleName}.dmfc")
    
    file(MAKE_DIRECTORY ${DMOD_DMF_DIR})
    file(MAKE_DIRECTORY ${DMOD_DMFC_DIR})

    add_custom_command(
        TARGET ${moduleName} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_OUTPUT} ${DMF_OUTPUT}
        COMMENT "Converting ELF to binary"
    )

    if(TODMFC)
        add_custom_command(
            TARGET ${moduleName} POST_BUILD
            COMMAND ${TODMFC} ${DMF_OUTPUT} ${DMFC_OUTPUT} ${DMOD_COMPRESSION_METHOD}
            COMMENT "Compressing the binary"
        )
    endif()

    set(DMD_OUTPUT "${DMOD_DMF_DIR}/${moduleName}.dmd")
    if(TODMD)
        # Check if version requirements file was created by dmod_link_modules
        get_target_property(VERSION_REQS_FILE ${moduleName} DMOD_VERSION_REQUIREMENTS_FILE)
        if(VERSION_REQS_FILE AND EXISTS "${VERSION_REQS_FILE}")
            # Pass version requirements file to todmd
            add_custom_command(
                TARGET ${moduleName} POST_BUILD
                COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT} -r ${VERSION_REQS_FILE}
                COMMENT "Generating dependencies file with version requirements"
            )
            message(STATUS "Will use version requirements from: ${VERSION_REQS_FILE}")
        else()
            # No version requirements file, use original command
            add_custom_command(
                TARGET ${moduleName} POST_BUILD
                COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT}
                COMMENT "Generating dependencies file"
            )
        endif()
    endif()

    # Print the source files
    message(STATUS "SOURCES for module ${moduleName}: ${SOURCES}")

    # ===========================================================================
    # Memory Analysis
    # ===========================================================================
    # Collect object files for memory analysis
    set(MODULE_OBJECT_FILES "")
    foreach(SOURCE_FILE ${SOURCES})
        # Preserve the source file's relative path structure for the object file
        set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${moduleName}.dir/${SOURCE_FILE}.o")
        list(APPEND MODULE_OBJECT_FILES "${OBJ_FILE}")
    endforeach()
    # Add the generated header object file
    list(APPEND MODULE_OBJECT_FILES "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${moduleName}.dir/${DMOD_MODULE_NAME_SNAKE_CASE}_header.c.o")
    
    # Convert list to string for passing to script (semicolon separated)
    string(REPLACE ";" "@@" MODULE_OBJECT_FILES_STR "${MODULE_OBJECT_FILES}")
    
    # Add custom command to show memory analysis after build
    add_custom_command(
        TARGET ${moduleName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${MODULE_OBJECT_FILES_STR}
            -DMODULE_NAME=${moduleName}
            -DDMOD_MODULE_NAME="${moduleName}"
            -DANALYSIS_TYPE=summary
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Analyzing memory usage for ${moduleName}"
    )
    
    # Add custom target for detailed symbol analysis
    add_custom_target(${moduleName}_memory_detailed
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${MODULE_OBJECT_FILES_STR}
            -DMODULE_NAME=${moduleName}
            -DDMOD_MODULE_NAME="${moduleName}"
            -DANALYSIS_TYPE=detailed
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Detailed memory analysis for ${moduleName}"
    )

    # ===========================================================================
    # Install target for modules
    # ===========================================================================
    # Determine installation directory for module files
    # Priority: Environment variable > Configuration variable > DMF_DIR default
    if(DEFINED ENV{DMOD_REPO_DIR})
        set(INSTALL_REPO_DIR $ENV{DMOD_REPO_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_REPO_DIR from environment: ${INSTALL_REPO_DIR}")
    elseif(DEFINED ENV{DMOD_DMF_DIR})
        set(INSTALL_REPO_DIR $ENV{DMOD_DMF_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_DMF_DIR from environment: ${INSTALL_REPO_DIR}")
    elseif(DEFINED DMOD_REPO_DIR)
        set(INSTALL_REPO_DIR ${DMOD_REPO_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_REPO_DIR from configuration: ${INSTALL_REPO_DIR}")
    else()
        set(INSTALL_REPO_DIR ${DMOD_DMF_DIR})
        message(STATUS "Module ${moduleName}: Will install to default location: ${INSTALL_REPO_DIR}")
    endif()

    # Install DMF file
    install(FILES ${DMF_OUTPUT}
        DESTINATION ${INSTALL_REPO_DIR}
        COMPONENT modules
        OPTIONAL
    )

    # Install DMFC file if compression is enabled
    if(TODMFC)
        install(FILES ${DMFC_OUTPUT}
            DESTINATION ${INSTALL_REPO_DIR}
            COMPONENT modules
            OPTIONAL
        )
    endif()

    # ===========================================================================
    # Release package creation from .dmr file
    # ===========================================================================
    if(DEFINED DMOD_DMR_PATH)
        # Resolve the dmr path relative to the caller's source directory
        if(NOT IS_ABSOLUTE "${DMOD_DMR_PATH}")
            set(DMOD_DMR_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DMOD_DMR_PATH}")
        else()
            set(DMOD_DMR_ABS_PATH "${DMOD_DMR_PATH}")
        endif()

        # Find mkdmrpkg tool
        find_program(MKDMRPKG
            mkdmrpkg
            HINTS ${DMOD_TOOLS_BIN_DIR}
        )

        dmod_setup_assets(${moduleName})
        set(MKDMRPKG_VIEWS_ARGS "")
        if(DMOD_VIEWS_ENABLED)
            set(MKDMRPKG_VIEWS_ARGS --views-dir "${DMOD_VIEWS_DIR}")
        endif()

        if(MKDMRPKG)
            set(DMOD_PACKAGES_DIR "${CMAKE_BINARY_DIR}/packages")
            set(DMOD_PKG_OUT_DIR "${DMOD_PACKAGES_DIR}/${moduleName}")
            set(DMOD_PKG_ZIP_PATH "${DMOD_PACKAGES_DIR}/${moduleName}.zip")
            set(DMOD_LOCAL_MANIFEST_PATH "${DMOD_PACKAGES_DIR}/manifest-local.dmm")

            # Create the packages directory
            file(MAKE_DIRECTORY "${DMOD_PACKAGES_DIR}")

            # Run mkdmrpkg to populate the package directory, then create zip archive
            add_custom_command(
                TARGET ${moduleName} POST_BUILD
                COMMAND ${MKDMRPKG} "${DMOD_DMR_ABS_PATH}"
                    -m "${moduleName}"
                    -r "${CMAKE_SOURCE_DIR}"
                    -b "${CMAKE_BINARY_DIR}"
                    --dmf-dir "${DMOD_DMF_DIR}"
                    ${MKDMRPKG_VIEWS_ARGS}
                    -o "${DMOD_PKG_OUT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E chdir "${DMOD_PKG_OUT_DIR}"
                    ${CMAKE_COMMAND} -E tar "cf" "${DMOD_PKG_ZIP_PATH}" --format=zip -- .
                WORKING_DIRECTORY "${DMOD_PACKAGES_DIR}"
                COMMENT "Creating release package and zip archive for ${moduleName}"
            )

            # Find todmm tool and generate local manifest from all zip packages
            find_program(TODMM
                todmm
                HINTS ${DMOD_TOOLS_BIN_DIR}
            )

            if(TODMM)
                # Create a shared target (once per project) that regenerates the local
                # manifest from ALL package zips after every module has been built.
                # Using a dedicated target (rather than per-module POST_BUILD) guarantees
                # that todmm runs only after ALL modules have created their zip archives,
                # so the manifest is always complete when local .dmd files are generated.
                if(NOT TARGET dmod_generate_manifest)
                    add_custom_target(dmod_generate_manifest ALL)
                    add_custom_command(
                        TARGET dmod_generate_manifest POST_BUILD
                        # todmm <folder> <base_url> --zip -o <output>
                        # For a local manifest, base_url is the local packages directory path
                        COMMAND ${TODMM} "${DMOD_PACKAGES_DIR}" "${DMOD_PACKAGES_DIR}" --zip -o "${DMOD_LOCAL_MANIFEST_PATH}"
                        COMMENT "Generating local manifest from all packages: ${DMOD_LOCAL_MANIFEST_PATH}"
                    )
                endif()

                # This module's zip must exist before the manifest is generated
                add_dependencies(dmod_generate_manifest ${moduleName})

                # Generate <module_name>-local.dmd using todmd AFTER the complete manifest
                # is ready (i.e., after all modules' zips have been collected by todmm).
                if(TODMD)
                    set(LOCAL_DMD_OUTPUT "${DMOD_DMF_DIR}/${moduleName}-local.dmd")

                    # A per-module ALL target that runs todmd once the manifest is complete
                    add_custom_target(${moduleName}_local_dmd ALL)
                    add_dependencies(${moduleName}_local_dmd dmod_generate_manifest)

                    get_target_property(VERSION_REQS_FILE ${moduleName} DMOD_VERSION_REQUIREMENTS_FILE)
                    if(VERSION_REQS_FILE AND EXISTS "${VERSION_REQS_FILE}")
                        add_custom_command(
                            TARGET ${moduleName}_local_dmd POST_BUILD
                            COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT} -r ${VERSION_REQS_FILE} --local-manifest "${DMOD_LOCAL_MANIFEST_PATH}"
                            COMMENT "Generating local dependencies file: ${LOCAL_DMD_OUTPUT}"
                        )
                    else()
                        add_custom_command(
                            TARGET ${moduleName}_local_dmd POST_BUILD
                            COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT} --local-manifest "${DMOD_LOCAL_MANIFEST_PATH}"
                            COMMENT "Generating local dependencies file: ${LOCAL_DMD_OUTPUT}"
                        )
                    endif()
                endif()
            else()
                message(WARNING "todmm not found - local manifest for packages will not be generated. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
            endif()
        else()
            message(WARNING "mkdmrpkg not found - release package for ${moduleName} will not be created. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
        endif()
    endif()
endfunction()

#
# Function to create a DMOD executable module
#
function(dmod_add_executable moduleName version)    
    set(SOURCES ${ARGN})

    dmod_create_module(${moduleName} ${version} Application ${SOURCES})
endfunction()

#
# Function to create a DMOD library module
#
function(dmod_add_library moduleName version)    
    set(SOURCES ${ARGN})

    dmod_create_module(${moduleName} ${version} Library ${SOURCES})
endfunction()

#
# Function to create a DMOD test module
#
# Usage:
#   dmod_add_test(moduleName version sources...)
#
# Creates an Application-type module with a pre-provided main() that
# automatically discovers and runs all test steps registered via
# DMOD_TEST_STEP().  Do NOT define main() yourself in the test sources.
#
# Lifecycle hooks dmod_test_setup() and dmod_test_teardown() are called
# before/after each step; define them in your test sources to override
# the default empty implementations.
#
function(dmod_add_test moduleName version)
    set(SOURCES ${ARGN})

    # Inject the pre-built test runner as an additional source file
    list(APPEND SOURCES ${DMOD_SRC_DIR}/module/dmod_test_main.c)

    dmod_create_module(${moduleName} ${version} Application ${SOURCES})
endfunction()

#
# Function to link external modules and download their headers
#
# Usage:
#   dmod_link_modules(target_name [PRIVATE|PUBLIC|INTERFACE]
#       module1@1.0
#       module2
#       module3@2.1
#   )
#
# This function downloads headers for the specified modules using dmf-get
# and adds them to the target's include directories.
#
# The visibility scope (PRIVATE, PUBLIC, or INTERFACE) can be specified and
# works the same way as target_include_directories. If not specified, PRIVATE is used.
#
function(dmod_link_modules targetName)
    # Parse arguments for visibility scope
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs PRIVATE PUBLIC INTERFACE)
    cmake_parse_arguments(DMOD_LINK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Determine visibility scope and module list
    set(VISIBILITY "PRIVATE")
    set(MODULE_SPECS "")
    
    if(DMOD_LINK_PRIVATE)
        set(VISIBILITY "PRIVATE")
        set(MODULE_SPECS ${DMOD_LINK_PRIVATE})
    elseif(DMOD_LINK_PUBLIC)
        set(VISIBILITY "PUBLIC")
        set(MODULE_SPECS ${DMOD_LINK_PUBLIC})
    elseif(DMOD_LINK_INTERFACE)
        set(VISIBILITY "INTERFACE")
        set(MODULE_SPECS ${DMOD_LINK_INTERFACE})
    else()
        # No visibility keyword specified, treat all unparsed args as module specs with PRIVATE visibility
        set(MODULE_SPECS ${DMOD_LINK_UNPARSED_ARGUMENTS})
    endif()
    
    # Check if target exists
    if(NOT TARGET ${targetName})
        message(FATAL_ERROR "dmod_link_modules: Target '${targetName}' does not exist")
    endif()
    
    # Find dmf-get tool
    find_program(DMF_GET 
        dmf-get
        HINTS ${DMOD_TOOLS_BIN_DIR}
        PATHS ENV PATH
    )
    
    # Save version requirements for this target to be used by todmd
    # This file will be passed to todmd when generating .dmd files
    set(VERSION_REQS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${targetName}_version_requirements.txt")
    file(WRITE "${VERSION_REQS_FILE}" "# Version requirements for ${targetName}\n")
    file(APPEND "${VERSION_REQS_FILE}" "# Generated by dmod_link_modules\n")
    file(APPEND "${VERSION_REQS_FILE}" "# Format: module_name@version\n\n")
    
    # Store the version requirements file path as a target property
    # This will be used by dmod_create_module to pass to todmd
    set_target_properties(${targetName} PROPERTIES
        DMOD_VERSION_REQUIREMENTS_FILE "${VERSION_REQS_FILE}"
    )
    
    if(NOT DMF_GET)
        message(WARNING "dmf-get tool not found. Skipping module headers download for ${targetName}. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
        # Still save version requirements even if dmf-get is not available
        foreach(MODULE_SPEC ${MODULE_SPECS})
            # Parse module name and version
            string(REGEX MATCH "^([^@]+)(@(.+))?$" MATCH_RESULT "${MODULE_SPEC}")
            set(MODULE_NAME "${CMAKE_MATCH_1}")
            set(MODULE_VERSION "${CMAKE_MATCH_3}")
            
            if(NOT MODULE_NAME)
                message(WARNING "Invalid module specification: ${MODULE_SPEC}")
                continue()
            endif()
            
            # Save version requirement to file if version is specified
            if(MODULE_VERSION)
                file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}@${MODULE_VERSION}\n")
                message(STATUS "Saved version requirement: ${MODULE_NAME}@${MODULE_VERSION}")
            else()
                file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}\n")
            endif()
        endforeach()
        return()
    endif()
    
    message(STATUS "Using dmf-get: ${DMF_GET}")
    
    # Get the default DMF directory for headers
    if(NOT DEFINED DMOD_DMF_DIR)
        set(DMOD_DMF_DIR "${CMAKE_BINARY_DIR}/dmf")
    endif()
    
    # Prepare dmf-get command options
    set(DMF_GET_OPTIONS "")
    
    # Add tools name if defined
    if(DEFINED DMOD_TOOLS_NAME)
        list(APPEND DMF_GET_OPTIONS "-t" "${DMOD_TOOLS_NAME}")
        message(STATUS "Using tools name: ${DMOD_TOOLS_NAME}")
    endif()
    
    # Add manifest URL if DMOD_DMM_URL is defined
    if(DEFINED DMOD_DMM_URL)
        list(APPEND DMF_GET_OPTIONS "-m" "${DMOD_DMM_URL}")
        message(STATUS "Using manifest: ${DMOD_DMM_URL}")
    endif()

    # Add CPU name if DMOD_CPU_NAME is defined and non-empty
    if(DEFINED DMOD_CPU_NAME AND NOT "${DMOD_CPU_NAME}" STREQUAL "")
        list(APPEND DMF_GET_OPTIONS "--cpu-name" "${DMOD_CPU_NAME}")
        message(STATUS "Using CPU name: ${DMOD_CPU_NAME}")
    endif()

    # Add CPU family if DMOD_CPU_FAMILY is defined and non-empty
    if(DEFINED DMOD_CPU_FAMILY AND NOT "${DMOD_CPU_FAMILY}" STREQUAL "")
        list(APPEND DMF_GET_OPTIONS "--cpu-family" "${DMOD_CPU_FAMILY}")
        message(STATUS "Using CPU family: ${DMOD_CPU_FAMILY}")
    endif()
    
    # Process each module specification
    foreach(MODULE_SPEC ${MODULE_SPECS})
        # Parse module name and version
        string(REGEX MATCH "^([^@]+)(@(.+))?$" MATCH_RESULT "${MODULE_SPEC}")
        set(MODULE_NAME "${CMAKE_MATCH_1}")
        set(MODULE_VERSION "${CMAKE_MATCH_3}")
        
        if(NOT MODULE_NAME)
            message(WARNING "Invalid module specification: ${MODULE_SPEC}")
            continue()
        endif()
        
        # Save version requirement to file if version is specified
        if(MODULE_VERSION)
            file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}@${MODULE_VERSION}\n")
            message(STATUS "Saved version requirement: ${MODULE_NAME}@${MODULE_VERSION}")
        else()
            file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}\n")
        endif()
        
        # Construct the header output directory
        # Note: dmf-get creates a subdirectory with the module name, so we don't add it here
        set(HEADER_OUTPUT_DIR "${DMOD_DMF_DIR}/inc")
        
        message(STATUS "Downloading headers for module: ${MODULE_NAME} -> ${HEADER_OUTPUT_DIR}")
        
        # Create the output directory
        file(MAKE_DIRECTORY "${HEADER_OUTPUT_DIR}")
        
        # Run dmf-get headers command with options
        execute_process(
            COMMAND ${DMF_GET} headers ${MODULE_SPEC} ${DMF_GET_OPTIONS} -o ${HEADER_OUTPUT_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE DMF_GET_RESULT
            ERROR_VARIABLE DMF_GET_ERROR
        )
        
        if(NOT DMF_GET_RESULT EQUAL 0)
            message(WARNING "Failed to download headers for ${MODULE_SPEC}: ${DMF_GET_ERROR}")
            continue()
        endif()
        
        # Add the header directory to the target's include directories with specified visibility
        # dmf-get creates MODULE_NAME subdirectory with an 'include' subdirectory inside
        # So the actual headers are in ${HEADER_OUTPUT_DIR}/${MODULE_NAME}/include
        set(ACTUAL_HEADER_DIR "${HEADER_OUTPUT_DIR}/${MODULE_NAME}/include")
        if(EXISTS "${ACTUAL_HEADER_DIR}")
            target_include_directories(${targetName} ${VISIBILITY} ${ACTUAL_HEADER_DIR})
            message(STATUS "Added include directory for ${MODULE_NAME} (${VISIBILITY}): ${ACTUAL_HEADER_DIR}")
        else()
            message(WARNING "Header directory does not exist after download: ${ACTUAL_HEADER_DIR}")
        endif()
    endforeach()
endfunction()

#
# Function to statically link "system modules" (precompiled static libraries)
# into a target
#
# Usage:
#   dmod_link_builtin(target_name [PRIVATE|PUBLIC|INTERFACE]
#       module1[@version]
#       module2[@version]
#   )
#
# Some DMOD modules (e.g. dmlist, dmosi-posix) are never turned into loadable
# .dmf files - they are "system modules", distributed as a precompiled static
# library plus headers (the "lib" resource in dmf-get/.dmr, see
# docs/dmf-get-tool.md and docs/dmr-file-format.md) meant to be built directly
# into a system-side binary (the loader itself, dmod-boot, host tools/tests).
# This function downloads both the headers and the static library for each
# module via dmf-get, adds the headers to the target's include directories,
# and statically links the library into targetName.
#
# These libraries define their dmod API functions in a way that always marks
# them as "used" to the compiler/linker, so simply linking the archive is
# enough to make their Built-in API available to whatever loads modules that
# call into them - no explicit symbol references are needed at the call site.
#
# Do NOT use this for regular DMOD modules (created with dmod_add_executable/
# dmod_add_library) - those must always link other modules dynamically via
# dmod_link_modules() instead, since modules are loaded/unloaded independently
# at runtime. This function is only for system-side binaries that are not
# themselves loaded as .dmf files.
#
# The visibility scope (PRIVATE, PUBLIC, or INTERFACE) works the same way as
# for dmod_link_modules()/target_link_libraries(). If not specified, PRIVATE
# is used.
#
function(dmod_link_builtin targetName)
    # Parse arguments for visibility scope
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs PRIVATE PUBLIC INTERFACE)
    cmake_parse_arguments(DMOD_LINK_BUILTIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Determine visibility scope and module list
    set(VISIBILITY "PRIVATE")
    set(MODULE_SPECS "")

    if(DMOD_LINK_BUILTIN_PRIVATE)
        set(VISIBILITY "PRIVATE")
        set(MODULE_SPECS ${DMOD_LINK_BUILTIN_PRIVATE})
    elseif(DMOD_LINK_BUILTIN_PUBLIC)
        set(VISIBILITY "PUBLIC")
        set(MODULE_SPECS ${DMOD_LINK_BUILTIN_PUBLIC})
    elseif(DMOD_LINK_BUILTIN_INTERFACE)
        set(VISIBILITY "INTERFACE")
        set(MODULE_SPECS ${DMOD_LINK_BUILTIN_INTERFACE})
    else()
        # No visibility keyword specified, treat all unparsed args as module specs with PRIVATE visibility
        set(MODULE_SPECS ${DMOD_LINK_BUILTIN_UNPARSED_ARGUMENTS})
    endif()

    # Check if target exists
    if(NOT TARGET ${targetName})
        message(FATAL_ERROR "dmod_link_builtin: Target '${targetName}' does not exist")
    endif()

    # Find dmf-get tool
    find_program(DMF_GET
        dmf-get
        HINTS ${DMOD_TOOLS_BIN_DIR}
        PATHS ENV PATH
    )

    if(NOT DMF_GET)
        message(WARNING "dmf-get tool not found. Skipping system library download for ${targetName}. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
        return()
    endif()

    message(STATUS "Using dmf-get: ${DMF_GET}")

    # Get the default DMF directory
    if(NOT DEFINED DMOD_DMF_DIR)
        set(DMOD_DMF_DIR "${CMAKE_BINARY_DIR}/dmf")
    endif()

    # Prepare dmf-get command options
    set(DMF_GET_OPTIONS "")

    if(DEFINED DMOD_TOOLS_NAME)
        list(APPEND DMF_GET_OPTIONS "-t" "${DMOD_TOOLS_NAME}")
        message(STATUS "Using tools name: ${DMOD_TOOLS_NAME}")
    endif()

    if(DEFINED DMOD_DMM_URL)
        list(APPEND DMF_GET_OPTIONS "-m" "${DMOD_DMM_URL}")
        message(STATUS "Using manifest: ${DMOD_DMM_URL}")
    endif()

    if(DEFINED DMOD_CPU_NAME AND NOT "${DMOD_CPU_NAME}" STREQUAL "")
        list(APPEND DMF_GET_OPTIONS "--cpu-name" "${DMOD_CPU_NAME}")
        message(STATUS "Using CPU name: ${DMOD_CPU_NAME}")
    endif()

    if(DEFINED DMOD_CPU_FAMILY AND NOT "${DMOD_CPU_FAMILY}" STREQUAL "")
        list(APPEND DMF_GET_OPTIONS "--cpu-family" "${DMOD_CPU_FAMILY}")
        message(STATUS "Using CPU family: ${DMOD_CPU_FAMILY}")
    endif()

    # Process each module specification
    foreach(MODULE_SPEC ${MODULE_SPECS})
        # Parse module name and version
        string(REGEX MATCH "^([^@]+)(@(.+))?$" MATCH_RESULT "${MODULE_SPEC}")
        set(MODULE_NAME "${CMAKE_MATCH_1}")

        if(NOT MODULE_NAME)
            message(WARNING "Invalid module specification: ${MODULE_SPEC}")
            continue()
        endif()

        # Download headers (same layout as dmod_link_modules())
        set(HEADER_OUTPUT_DIR "${DMOD_DMF_DIR}/inc")
        file(MAKE_DIRECTORY "${HEADER_OUTPUT_DIR}")

        message(STATUS "Downloading headers for system module: ${MODULE_NAME} -> ${HEADER_OUTPUT_DIR}")

        execute_process(
            COMMAND ${DMF_GET} headers ${MODULE_SPEC} ${DMF_GET_OPTIONS} -o ${HEADER_OUTPUT_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE DMF_GET_HEADERS_RESULT
            ERROR_VARIABLE DMF_GET_HEADERS_ERROR
        )

        if(NOT DMF_GET_HEADERS_RESULT EQUAL 0)
            message(WARNING "Failed to download headers for ${MODULE_SPEC}: ${DMF_GET_HEADERS_ERROR}")
        else()
            set(ACTUAL_HEADER_DIR "${HEADER_OUTPUT_DIR}/${MODULE_NAME}/include")
            if(EXISTS "${ACTUAL_HEADER_DIR}")
                target_include_directories(${targetName} ${VISIBILITY} ${ACTUAL_HEADER_DIR})
                message(STATUS "Added include directory for ${MODULE_NAME} (${VISIBILITY}): ${ACTUAL_HEADER_DIR}")
            else()
                message(WARNING "Header directory does not exist after download: ${ACTUAL_HEADER_DIR}")
            endif()
        endif()

        # Download the precompiled static library
        set(LIB_OUTPUT_DIR "${DMOD_DMF_DIR}/lib")
        file(MAKE_DIRECTORY "${LIB_OUTPUT_DIR}")

        message(STATUS "Downloading static library for system module: ${MODULE_NAME} -> ${LIB_OUTPUT_DIR}")

        execute_process(
            COMMAND ${DMF_GET} lib ${MODULE_SPEC} ${DMF_GET_OPTIONS} -o ${LIB_OUTPUT_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE DMF_GET_LIB_RESULT
            ERROR_VARIABLE DMF_GET_LIB_ERROR
        )

        if(NOT DMF_GET_LIB_RESULT EQUAL 0)
            message(WARNING "Failed to download static library for ${MODULE_SPEC}: ${DMF_GET_LIB_ERROR}")
            continue()
        endif()

        # The archive file name follows the module's snake_case form (hyphens
        # replaced with underscores, e.g. "dmosi-posix" -> "libdmosi_posix.a"),
        # matching how these modules' own CMakeLists.txt name the CMake library
        # target that produces them (see dmlist/CMakeLists.txt and
        # dmosi-posix/CMakeLists.txt, and their respective .dmr "lib=" entries).
        to_snake_case(${MODULE_NAME} MODULE_NAME_SNAKE_CASE)
        set(ACTUAL_LIB_PATH "${LIB_OUTPUT_DIR}/${MODULE_NAME}/lib/lib${MODULE_NAME_SNAKE_CASE}.a")

        if(EXISTS "${ACTUAL_LIB_PATH}")
            # A plain link only pulls object files out of a static archive to
            # resolve symbols someone else already references. Nothing in a
            # system binary like dmod_loader calls these functions directly -
            # they exist purely so a *dynamically loaded* module can resolve
            # them later through the Built-in API - so without forcing the
            # whole archive in, the linker drops every object file here and
            # the symbols never make it into the binary at all.
            target_link_libraries(${targetName} ${VISIBILITY}
                -Wl,--whole-archive ${ACTUAL_LIB_PATH} -Wl,--no-whole-archive
            )
            message(STATUS "Statically linked system module ${MODULE_NAME} (${VISIBILITY}): ${ACTUAL_LIB_PATH}")
        else()
            message(WARNING "Static library not found after download: ${ACTUAL_LIB_PATH}")
        endif()
    endforeach()
endfunction()

#
# Function to create a DMOD system
#
function(dmod_add_tool toolName)
    set(SOURCES ${ARGN})

    add_executable(${toolName} ${SOURCES})

    dmod_set_current_allocator(${toolName})

    target_include_directories(${toolName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(${toolName} dmod)
    target_link_options(${toolName} PRIVATE -L ${DMOD_SCRIPTS_DIR} -T ${DMOD_SCRIPTS_DIR}/tool.ld)

    # Move the binary to the tools directory
    add_custom_command(
        TARGET ${toolName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${toolName}> ${DMOD_TOOLS_BIN_DIR}/${toolName}
        COMMENT "Copying the binary to the tools directory"
    )

    # Install the tool to the system bin directory
    install(TARGETS ${toolName}
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
endfunction()
