cmake_minimum_required(VERSION 2.8.12)
project(angelscript_addon)

include_directories(../angelscript/include)

add_library(angelscript_addon

scriptbuilder/scriptbuilder.cpp
scripthandle/scripthandle.cpp
scriptstdstring/scriptstdstring.cpp
weakref/weakref.cpp
)
