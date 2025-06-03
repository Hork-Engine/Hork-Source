cmake_minimum_required(VERSION 3.18)

project(Hork-Samples)

#include_directories(Hork-Source)
#include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)
include_directories(Source)

set(HK_ASSET_DATA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Data")
set(HK_ASSET_DATA_INSTALL_PATH "${HK_PROJECT_BUILD_PATH}/Data")

make_link(${HK_ASSET_DATA_PATH} ${HK_ASSET_DATA_INSTALL_PATH})

add_subdirectory_with_folder("Samples" 00_Empty)
add_subdirectory_with_folder("Samples" 01_HelloWorld)
add_subdirectory_with_folder("Samples" 02_FirstPersonShooter)
add_subdirectory_with_folder("Samples" 03_ThirdPerson)
add_subdirectory_with_folder("Samples" 04_RenderToTexture)
add_subdirectory_with_folder("Samples" 05_NavMesh)
add_subdirectory_with_folder("Samples" 06_SkeletalAnimation)
add_subdirectory_with_folder("Samples" 07_IesProfiles)
add_subdirectory_with_folder("Samples" 08_MoviePlayer)
add_subdirectory_with_folder("Samples" 09_GifPlayer)
add_subdirectory_with_folder("Samples" 10_InverseKinematics)
add_subdirectory_with_folder("Samples" 11_WaterVolume)
