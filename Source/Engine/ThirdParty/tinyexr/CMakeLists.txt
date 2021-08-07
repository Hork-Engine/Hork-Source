cmake_minimum_required(VERSION 3.5)
project(tinyexr)

if (MSVC)
    set(FASTLZ_COMPILE_FLAGS /wd5208)
endif()

add_library(tinyexr STATIC tinyexr.cpp tinyexr.h)
