cmake_minimum_required(VERSION 2.8)

set( PROJECT glutess )

project( ${PROJECT} )

set( SOURCES
dict.c
geom.c
main.c
memalloc.c
mesh.c
normal.c
priorityq.c
priorityq-heap.c
render.c
sweep.c
tess.c
tessellate.c
tessmono.c
)

set( HEADERS
dict.h
dict-list.h
geom.h
gluos.h
glutess.h
memalloc.h
mesh.h
normal.h
priorityq.h
priorityq-heap.h
priorityq-sort.h
render.h
sweep.h
tess.h
tessellate.h
tessmono.h
)

source_group( "Source" FILES ${SOURCES} ${HEADERS} )

setup_msvc_runtime_library()

add_library( ${PROJECT} ${HEADERS} ${SOURCES} )
