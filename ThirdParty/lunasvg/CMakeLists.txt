cmake_minimum_required(VERSION 3.3)

project(lunasvg VERSION 2.3.2 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

add_library(lunasvg)

add_subdirectory(3rdparty/software)
add_subdirectory(3rdparty/plutovg)


target_sources(lunasvg 
PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/lunasvg.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/element.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/property.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/parser.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/layoutcontext.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/canvas.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/clippathelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/defselement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/geometryelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/graphicselement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/maskelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/markerelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/paintelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/stopelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/styledelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/styleelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/svgelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/symbolelement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/useelement.cpp"
)

target_include_directories(lunasvg
PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
)
