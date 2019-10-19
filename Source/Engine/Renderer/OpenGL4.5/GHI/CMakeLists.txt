cmake_minimum_required(VERSION 2.8)

set( PROJECT GHI )

project( ${PROJECT} )

setup_msvc_runtime_library()

file( GLOB Sources *.h *.cpp )
source_group( "Sources" FILES ${Sources} )

add_library( ${PROJECT} ${Sources} )
