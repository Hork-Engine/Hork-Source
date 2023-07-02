cmake_minimum_required(VERSION 3.5)
project(glutess)

add_library(glutess STATIC
dict-list.h
dict.c
dict.h
geom.c
geom.h
gluos.h
glutess.h
LICENSE
main.c
memalloc.c
memalloc.h
mesh.c
mesh.h
normal.c
normal.h
priorityq-heap.h
priorityq-heap_impl.h
priorityq-sort.h
priorityq.c
priorityq.h
render.c
render.h
sweep.c
sweep.h
tess.c
tess.h
tessellate.c
tessellate.h
tessmono.c
tessmono.h
)
