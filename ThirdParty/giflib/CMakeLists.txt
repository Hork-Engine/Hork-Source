cmake_minimum_required(VERSION 3.5)
project(giflib)

add_library(giflib STATIC
dgif_lib.c
egif_lib.c
gif_err.c
gif_font.c
gif_hash.c
gif_hash.h
gif_lib.h
gif_lib_private.h
gifalloc.c
openbsd-reallocarray.c
)
