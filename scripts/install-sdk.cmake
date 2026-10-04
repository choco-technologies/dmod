include(CMakePackageConfigHelpers)

# The SDK contains target-neutral sources, not host-built static archives.
set(DMOD_SDK_INSTALL_DIR share/dmod)
set(DMOD_SDK_INCLUDE_DIR include/dmod)
set(DMOD_CMAKE_INSTALL_DIR share/cmake/dmod)
configure_package_config_file(
    "${DMOD_DIR}/cmake/dmodConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dmodConfig.cmake"
    INSTALL_DESTINATION "${DMOD_CMAKE_INSTALL_DIR}"
    PATH_VARS DMOD_SDK_INSTALL_DIR DMOD_SDK_INCLUDE_DIR)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/dmodConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dmodConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dmodConfigVersion.cmake"
    DESTINATION "${DMOD_CMAKE_INSTALL_DIR}" COMPONENT development)
install(DIRECTORY "${DMOD_DIR}/inc/"
    DESTINATION "${DMOD_SDK_INCLUDE_DIR}" COMPONENT development
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES "${DMOD_DIR}/dmod-defaults.cmake" "${DMOD_DIR}/dmod-config.h.in"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}" COMPONENT development)
install(DIRECTORY "${DMOD_DIR}/src/common" "${DMOD_DIR}/src/module"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}/src" COMPONENT development
    FILES_MATCHING PATTERN "*.c" PATTERN "CMakeLists.txt")
install(DIRECTORY "${DMOD_DIR}/lib/third-party/FastLZ"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}/lib/third-party" COMPONENT development
    FILES_MATCHING PATTERN "*.c" PATTERN "*.h" PATTERN "CMakeLists.txt"
    PATTERN "LICENSE.MIT")
install(DIRECTORY "${DMOD_DIR}/scripts/"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}/scripts" COMPONENT development
    FILES_MATCHING PATTERN "CMakeLists.txt" PATTERN "*.in" PATTERN "*.ld"
    PATTERN "memory_analysis.cmake")
install(DIRECTORY "${DMOD_DIR}/cmake/runtime"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}/cmake" COMPONENT development)
install(FILES "${DMOD_DIR}/cmake/select-profile.cmake"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}/cmake" COMPONENT development)
install(DIRECTORY "${DMOD_DIR}/configs/"
    DESTINATION "${DMOD_SDK_INSTALL_DIR}/configs" COMPONENT development
    FILES_MATCHING PATTERN "*.cmake")
install(FILES "${DMOD_DIR}/license.md"
    DESTINATION share/doc/dmod-dev RENAME copyright COMPONENT development)
install(FILES "${DMOD_DIR}/lib/third-party/FastLZ/LICENSE.MIT"
    DESTINATION share/doc/dmod-dev RENAME copyright.FastLZ COMPONENT development)
install(FILES "${DMOD_DIR}/docs/development-package.md"
    DESTINATION share/doc/dmod-dev COMPONENT development)
