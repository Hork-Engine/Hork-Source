cmake_minimum_required(VERSION 3.5)
project(webp)

file( GLOB_RECURSE SRC src/*.h src/*.c sharpyuv/*.h sharpyuv/*.c )

include_directories(.)

add_library(webp STATIC ${SRC})
