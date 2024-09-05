cmake_minimum_required(VERSION 3.18)

project(HorkSource)

include(hork_config.cmake)

include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)
include_directories(.)

add_subdirectory_with_folder("BuildTools" BuildTools)

add_subdirectory(ThirdParty)
add_subdirectory(Hork)
