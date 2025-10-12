cmake_minimum_required(VERSION 3.18)

# Build DIF interface definition
add_subdirectory(difs)

# Build DIF implementations
add_subdirectory(fatfs)
add_subdirectory(flashfs)

# Build VFS that uses the DIF
add_subdirectory(vfs)
