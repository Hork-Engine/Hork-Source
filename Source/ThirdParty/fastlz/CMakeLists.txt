cmake_minimum_required(VERSION 2.8)

set( PROJECT fastlz )

project( ${PROJECT} )

setup_msvc_runtime_library()

set( SOURCES fastlz.c )
set( HEADERS fastlz.h )

source_group( "Source" FILES ${SOURCES} ${HEADERS} )

add_library( ${PROJECT} ${HEADERS} ${SOURCES} )
