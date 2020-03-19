cmake_minimum_required(VERSION 2.8)

set( PROJECT Recast )

project( ${PROJECT} )

setup_msvc_runtime_library()

file( GLOB DebugUtils DebugUtils/Source/*.cpp DebugUtils/Include/*.h )
file( GLOB Detour Detour/Source/*.cpp Detour/Include/*.h )
file( GLOB DetourCrowd DetourCrowd/Source/*.cpp DetourCrowd/Include/*.h )
file( GLOB DetourTileCache DetourTileCache/Source/*.cpp DetourTileCache/Include/*.h )
file( GLOB Recast Recast/Source/*.cpp Recast/Include/*.h )

source_group( "DebugUtils" FILES ${DebugUtils} )
source_group( "Detour" FILES ${Detour} )
source_group( "DetourCrowd" FILES ${DetourCrowd} )
source_group( "DetourTileCache" FILES ${DetourTileCache} )
source_group( "Recast" FILES ${Recast} )

add_library( ${PROJECT} ${DebugUtils}
						${Detour}
						${DetourCrowd}
						${DetourTileCache}
						${Recast} )

