#cmake_minimum_required(VERSION 3.8)

set( PROJECT 04_Terrain )

file( GLOB Code *.h *.cpp )
file( GLOB Resources res/Angie.rc res/icon1.ico res/resource.h )

source_group( "" FILES ${Code} )
source_group( "res" FILES ${Resources} )

set( SRCS
${Code}
${Resources}
)

setup_msvc_runtime_library()

if(WIN32)
add_executable( ${PROJECT} WIN32 ${SRCS} )
else()
add_executable( ${PROJECT} ${SRCS} )
endif()

target_link_libraries( ${PROJECT} AngieEngine )

#set_property( TARGET ${PROJECT} PROPERTY POSITION_INDEPENDENT_CODE ON )

target_compile_definitions(${PROJECT} PUBLIC ${AN_COMPILER_DEFINES})
target_compile_options(${PROJECT} PUBLIC ${AN_COMPILER_FLAGS})
