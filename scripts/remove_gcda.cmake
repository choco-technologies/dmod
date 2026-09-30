# Removes all *.gcda files from ${DIR}
file(GLOB_RECURSE gcda_files "${DIR}/*.gcda")
if(gcda_files)
    file(REMOVE ${gcda_files})
endif()
