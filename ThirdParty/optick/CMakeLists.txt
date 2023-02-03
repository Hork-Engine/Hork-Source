cmake_minimum_required(VERSION 3.2)
project(Optick LANGUAGES CXX VERSION 1.3.0)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 11)


# hacks for standalone builds with visual studio
if(MSVC AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
	message(STATUS "Standalone build")
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Build")
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
	set(standalone ON)
else()
	set(standalone OFF)
endif()


# Sources
file(GLOB OPTICK_SRC "src/*.*")
source_group("OptickCore" FILES ${OPTICK_SRC})


# Enabled
option(OPTICK_ENABLED "Enable profiling with Optick" ON)

if(NOT OPTICK_ENABLED)
	message(STATUS "Optick is disabled")
	# add dummy target as a replacement
	add_library(OptickCore STATIC ${OPTICK_SRC})
	target_include_directories(OptickCore PUBLIC "src")
	target_compile_definitions(OptickCore PUBLIC USE_OPTICK=0)
	return()
endif()


# Options
option(OPTICK_USE_VULKAN "Built-in support for Vulkan" OFF)
option(OPTICK_USE_D3D12 "Built-in support for DirectX 12" OFF)

# OptickCore
add_library(OptickCore SHARED ${OPTICK_SRC})
target_include_directories(OptickCore
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
set_target_properties(OptickCore 
	PROPERTIES 
		PUBLIC_HEADER "${CMAKE_CURRENT_LIST_DIR}/src/optick.h;${CMAKE_CURRENT_LIST_DIR}/src/optick.config.h"
		DEBUG_POSTFIX d # So that we can install debug and release side by side
)
target_compile_definitions(OptickCore PRIVATE OPTICK_EXPORTS=1)
if(OPTICK_USE_VULKAN)
	message(STATUS "Optick uses Vulkan")
	find_package(Vulkan REQUIRED)
	target_link_libraries(OptickCore PRIVATE Vulkan::Vulkan)
else()
	target_compile_definitions(OptickCore PRIVATE OPTICK_ENABLE_GPU_VULKAN=0)
endif()
if(OPTICK_USE_D3D12)
	message(STATUS "Optick uses DirectX 12")
	target_link_libraries(OptickCore PRIVATE "d3d12.lib" "dxgi.lib")
else()
	target_compile_definitions(OptickCore PRIVATE OPTICK_ENABLE_GPU_D3D12=0)
endif()
if(OPTICK_USE_D3D12 OR OPTICK_USE_VULKAN)
	target_compile_definitions(OptickCore PRIVATE OPTICK_ENABLE_GPU=1)
else()
	target_compile_definitions(OptickCore PRIVATE OPTICK_ENABLE_GPU=0)
endif()

if(MSVC)
	# temporary solution to unblock C++17 users
	target_compile_definitions(OptickCore PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()

set(EXTRA_LIBS ${EXTRA_LIBS} OptickCore)
if(NOT MSVC)
	set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif()
