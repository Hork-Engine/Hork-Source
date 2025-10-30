cmake_minimum_required(VERSION 3.18)

project(Hork-Engine)

option(HK_BUILD_SAMPLES "Enable samples" ON)

include(hork_config.cmake)

add_compile_options(
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-fno-rtti>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-fno-rtti>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/GR->"
)

include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)
include_directories(.)

add_subdirectory_with_folder("BuildTools" BuildTools)

add_subdirectory(ThirdParty)

add_subdirectory_with_folder("Hork" Hork)

if (HK_BUILD_SAMPLES)
add_subdirectory(Samples)
endif()

add_subdirectory(Tools)
