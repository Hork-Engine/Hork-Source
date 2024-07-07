cmake_minimum_required(VERSION 3.23.1)

# Defines the project's name
project(ozz)

# Current version
set(OZZ_VERSION_MAJOR 0)
set(OZZ_VERSION_MINOR 15)
set(OZZ_VERSION_PATCH 0)
set(OZZ_VERSION ${OZZ_VERSION_MAJOR}.${OZZ_VERSION_MINOR}.${OZZ_VERSION_PATCH})

# Add project build options
option(BUILD_SHARED_LIBS "Build ozz as shared libraries" OFF)
option(ozz_build_simd_ref "Force SIMD math reference implementation" OFF)
option(ozz_build_postfix "Use per config postfix name" ON)
option(ozz_build_msvc_rt_dll "Select msvc DLL runtime library" ON)

# Include ozz cmake parameters and scripts
include(CheckCXXCompilerFlag)
include(${PROJECT_SOURCE_DIR}/build-utils/cmake/compiler_settings.cmake)
include(${PROJECT_SOURCE_DIR}/build-utils/cmake/fuse_target.cmake)

# Starts building the sources tree
add_subdirectory(src)
