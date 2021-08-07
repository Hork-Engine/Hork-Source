cmake_minimum_required(VERSION 3.5)
project(miniaudio)

add_library(miniaudio STATIC miniaudio.c miniaudio.h stb_vorbis.h)
