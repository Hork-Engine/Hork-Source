cmake_minimum_required(VERSION 3.5)
project(fastlz)

if (MSVC)
    set(FASTLZ_COMPILE_FLAGS /wd4244)
endif()

add_library(fastlz STATIC fastlz.c fastlz.h)
target_compile_options(fastlz PRIVATE ${FASTLZ_COMPILE_FLAGS})
