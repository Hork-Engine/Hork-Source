cmake_minimum_required(VERSION 2.8)

set( PROJECT minizip )

project( ${PROJECT} )

set( SOURCES ioapi.c iowin32.c mztools.c unzip.c zip.c )
set( HEADERS crypt.h ioapi.h iowin32.h mztools.h unzip.h zip.h )

source_group( "Source" FILES ${SOURCES} ${HEADERS} )

setup_msvc_runtime_library()

add_library( ${PROJECT} ${HEADERS} ${SOURCES} )

target_link_libraries( ${PROJECT} ${ZLIB_LIBRARY} )

