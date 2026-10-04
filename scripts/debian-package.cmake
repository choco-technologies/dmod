# Package only the existing tools install component, excluding SDK headers,
# static libraries, examples and any install rules from test dependencies.
set(CPACK_GENERATOR DEB)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL tools)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

set(CPACK_PACKAGE_NAME "dmod")
set(CPACK_PACKAGE_VENDOR "Choco Technologies")
set(CPACK_PACKAGE_CONTACT "Patryk Kubiak <patryk.kubiak90@gmail.com>")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DMOD host tools and dynamic module loader")
set(CPACK_PACKAGE_DESCRIPTION
    "Tools for creating, compressing, locating and installing DMOD modules, and the dmod_loader host runtime.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/choco-technologies/dmod")
set(CPACK_RESOURCE_FILE_LICENSE "${DMOD_DIR}/license.md")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
# Resolve libc/libcurl (and any other linked libraries) on the build distro.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# dmf-get invokes unzip when installing module and resource archives.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "unzip")

install(FILES "${DMOD_DIR}/license.md"
    DESTINATION share/doc/dmod RENAME copyright COMPONENT tools)
install(FILES "${DMOD_DIR}/docs/debian-package.md"
    DESTINATION share/doc/dmod COMPONENT tools)

include(CPack)
