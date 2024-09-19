cmake_minimum_required(VERSION 3.18)

project(Hork-Samples)

#include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)
#include_directories(Source)

add_subdirectory_with_folder("Tools" MeshImporter)
add_subdirectory_with_folder("Tools" TextureImporter)
