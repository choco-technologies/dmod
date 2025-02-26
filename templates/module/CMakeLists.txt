

# #############################################################
#   Creation of the library module using the template
# #############################################################
set(DMOD_MODULE_NAME  MyLibrary)
set(DMOD_AUTHOR_NAME  "John Doe")
set(DMOD_STACK_SIZE   1024)
set(INPUT_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/library)
set(OUTPUT_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/output/library)

file(MAKE_DIRECTORY ${OUTPUT_DIR})

configure_file(
    ${INPUT_DIR}/CMakeLists.txt
    ${OUTPUT_DIR}/CMakeLists.txt
)

configure_file(
    ${INPUT_DIR}/main.c
    ${OUTPUT_DIR}/${DMOD_MODULE_NAME}.c
)

add_subdirectory(output/library)

# #############################################################
#   Creation of the application module using the template
# #############################################################
set(DMOD_MODULE_NAME  MyApplication)
set(DMOD_AUTHOR_NAME  "John Doe")
set(DMOD_STACK_SIZE   1024)
set(INPUT_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/application)
set(OUTPUT_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/output/application)

file(MAKE_DIRECTORY ${OUTPUT_DIR})

configure_file(
    ${INPUT_DIR}/CMakeLists.txt
    ${OUTPUT_DIR}/CMakeLists.txt
)

configure_file(
    ${INPUT_DIR}/main.c
    ${OUTPUT_DIR}/${DMOD_MODULE_NAME}.c
)

add_subdirectory(output/application)




