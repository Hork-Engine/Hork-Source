cmake_minimum_required(VERSION 2.8)

set( PROJECT imgui )

project( ${PROJECT} )

file( GLOB ImguiSources *.h *.cpp )

source_group( "imgui" FILES ${ImguiSources} )

setup_msvc_runtime_library()

add_library( ${PROJECT} ${ImguiSources} )
