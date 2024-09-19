cmake_minimum_required(VERSION 3.18)

project(Hork-Engine)

option(HK_BUILD_SAMPLES "Enable samples" ON)

include(hork_config.cmake)

include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)
include_directories(.)

add_subdirectory_with_folder("BuildTools" BuildTools)

add_subdirectory(ThirdParty)

add_subdirectory_with_folder("Hork" Hork)

if (HK_BUILD_SAMPLES)
add_subdirectory(Samples)
endif()

add_subdirectory(Tools)
