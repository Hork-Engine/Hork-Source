cmake_minimum_required(VERSION 2.8)

set( PROJECT poly2tri )

project( ${PROJECT} )

set( SOURCES
common/shapes.cc
sweep/advancing_front.cc
sweep/cdt.cc
sweep/sweep.cc
sweep/sweep_context.cc
)

set( HEADERS
poly2tri.h
common/shapes.h
common/utils.h
sweep/advancing_front.h
sweep/cdt.h
sweep/sweep.h
sweep/sweep_context.h
)

source_group( "Source" FILES ${SOURCES} ${HEADERS} )

setup_msvc_runtime_library()

add_library( ${PROJECT} ${HEADERS} ${SOURCES} )
