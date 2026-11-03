if(DMOD_SYSTEM)
    add_subdirectory(lib)
    add_subdirectory(system)
endif()
# The test target can run local-package integration tests once these tools exist.
if(TARGET check AND TARGET dmf-get AND TARGET mkdmrpkg)
    add_dependencies(check dmf-get mkdmrpkg)
endif()
