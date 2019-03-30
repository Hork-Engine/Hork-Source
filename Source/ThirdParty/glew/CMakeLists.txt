cmake_minimum_required(VERSION 2.8)

set( PROJECT glew )

project( ${PROJECT} )

set( SOURCES src/glew.c )
set( HEADERS
 include/GL/glew.h
 include/GL/glxew.h
 include/GL/wglew.h
)

source_group( "Source" FILES ${SOURCES} ${HEADERS} )


setup_msvc_runtime_library()

add_library( ${PROJECT} ${HEADERS} ${SOURCES} )
