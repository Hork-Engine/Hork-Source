cmake_minimum_required(VERSION 2.8)

set( PROJECT Clipper )

project( ${PROJECT} )

set( SOURCES clipper.cpp )
set( HEADERS clipper.hpp )

source_group( "Source" FILES ${SOURCES} ${HEADERS} )

setup_msvc_runtime_library()

add_library( ${PROJECT} ${HEADERS} ${SOURCES} )
