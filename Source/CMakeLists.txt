set( PROJECT HorkEngine )

project( ${PROJECT} )

setup_msvc_runtime_library()

include_directories( . )

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        get_target_property(_type ${_target} TYPE)
        if (NOT ${_type} STREQUAL "INTERFACE_LIBRARY")
            set_target_properties(${_target} PROPERTIES FOLDER "${_folder_name}")
        endif()
    endforeach()
endfunction()

include_directories( ThirdParty )

#
# GLEW
#
add_definitions( -DGLEW_STATIC )
add_definitions( -DGLEW_NO_GLU )

find_package(OpenGL REQUIRED)

message(STATUS "======================================================================")
message(STATUS "USING SDL2")
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
set(SDL_DIRECTX OFF CACHE BOOL "" FORCE)
#set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
#set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN   ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL    ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_UNINSTALL  ON CACHE BOOL "" FORCE)
include_directories(ThirdParty/SDL2/include)
add_subdirectory_with_folder("ThirdParty/SDL" ThirdParty/SDL2)

message(STATUS "======================================================================")
message(STATUS "USING MIMALLOC")
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
include_directories(ThirdParty/mimalloc/include)
add_subdirectory_with_folder("ThirdParty/mimalloc" ThirdParty/mimalloc)

message(STATUS "======================================================================")
message(STATUS "USING EASTL")
include_directories(ThirdParty/EASTL/include)
include_directories(ThirdParty/EASTL/EABase/include/Common)
add_subdirectory_with_folder("ThirdParty/EASTL" ThirdParty/EASTL)

message(STATUS "======================================================================")
message(STATUS "USING FMT")
include_directories( ThirdParty/fmt/include )
add_subdirectory( "ThirdParty/fmt" )

message(STATUS "======================================================================")
message(STATUS "USING BULLET PHYSICS")
set( USE_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set( USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD OFF CACHE BOOL "" FORCE)
set( BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
include_directories( ThirdParty/bullet3/src )
add_subdirectory_with_folder("ThirdParty/BulletPhysics" ThirdParty/bullet3)

set (HK_BULLET_LIBS
BulletSoftBody
BulletDynamics
BulletCollision
LinearMath
#Bullet2FileLoader
#BulletFileLoader
#Bullet3Collision
#Bullet3Common
#Bullet3Dynamics
#Bullet3Geometry
#Bullet3OpenCL_clew
BulletInverseDynamics
#BulletInverseDynamicsUtils
#BulletWorldImporter
#BulletXmlWorldImporter
#ConvexDecomposition
#GIMPACTUtils
)


message(STATUS "======================================================================")
message(STATUS "USING HACD")
include_directories( ThirdParty/HACD )
add_subdirectory_with_folder( "ThirdParty/HACD" ThirdParty/HACD )

message(STATUS "======================================================================")
message(STATUS "USING VHACD")
add_subdirectory_with_folder( "ThirdParty/VHACD" ThirdParty/VHACD )

message(STATUS "======================================================================")
message(STATUS "USING RECAST NAVIGATION")
include_directories( ThirdParty/recastnavigation/DebugUtils/Include )
include_directories( ThirdParty/recastnavigation/Detour/Include )
include_directories( ThirdParty/recastnavigation/DetourCrowd/Include )
include_directories( ThirdParty/recastnavigation/DetourTileCache/Include )
include_directories( ThirdParty/recastnavigation/Recast/Include )
add_subdirectory_with_folder( "ThirdParty/recastnavigation" ThirdParty/recastnavigation )

message(STATUS "======================================================================")
message(STATUS "USING muFFT")
add_subdirectory_with_folder( "ThirdParty/muFFT" ThirdParty/muFFT )

message(STATUS "======================================================================")
message(STATUS "USING MINIAUDIO")
add_subdirectory_with_folder( "ThirdParty/miniaudio" ThirdParty/miniaudio )

message(STATUS "======================================================================")
message(STATUS "USING MINIZ")
add_subdirectory_with_folder( "ThirdParty/miniz" ThirdParty/miniz )

message(STATUS "======================================================================")
message(STATUS "USING FASTLZ")
add_subdirectory_with_folder( "ThirdParty/fastlz" ThirdParty/fastlz )

message(STATUS "======================================================================")
message(STATUS "USING GLUTESS")
add_subdirectory_with_folder( "ThirdParty/glutess" ThirdParty/glutess )

message(STATUS "======================================================================")
message(STATUS "USING CLIPPER")
add_subdirectory_with_folder( "ThirdParty/clipper" ThirdParty/clipper )

message(STATUS "======================================================================")
message(STATUS "USING TINYEXR")
add_subdirectory_with_folder( "ThirdParty/tinyexr" ThirdParty/tinyexr )

message(STATUS "======================================================================")
message(STATUS "USING CGLTF")
add_subdirectory_with_folder( "ThirdParty/cgltf" ThirdParty/cgltf )

message(STATUS "======================================================================")
message(STATUS "USING BC7ENC")
add_subdirectory_with_folder( "ThirdParty/bc7enc_rdo" ThirdParty/bc7enc_rdo )

message(STATUS "======================================================================")
message(STATUS "USING BCDEC")
add_subdirectory_with_folder( "ThirdParty/bcdec" ThirdParty/bcdec )

message(STATUS "======================================================================")
message(STATUS "USING LUNASVG")
add_subdirectory_with_folder( "ThirdParty/lunasvg" ThirdParty/lunasvg )

message(STATUS "======================================================================")
message(STATUS "USING ANGELSCRIPT")
include_directories(ThirdParty/angelscript/include)
include_directories(ThirdParty/angelscript_addon)
add_subdirectory(ThirdParty/angelscript/projects/cmake angelscript)
add_subdirectory(ThirdParty/angelscript_addon angelscript_addon)
set_target_properties(angelscript PROPERTIES FOLDER ThirdParty/angelscript)
set_target_properties(angelscript_addon PROPERTIES FOLDER ThirdParty/angelscript)



#---------------------------------------------------------------------
# Platform module
file( GLOB Platform Platform/*.h Platform/*.cpp )
file( GLOB PlatformMemory Platform/Memory/*.h Platform/Memory/*.cpp )

source_group( "Platform" FILES ${Platform} )
source_group( "Platform\\Memory" FILES ${PlatformMemory} )

#---------------------------------------------------------------------
# Core module
file( GLOB Core Core/*.h Core/*.cpp Core/*.c )

source_group( "Core" FILES ${Core} )

#---------------------------------------------------------------------
# Containers module
file( GLOB Containers Containers/*.h Containers/*.cpp )
source_group( "Containers" FILES ${Containers} )

#---------------------------------------------------------------------
# Geometry module
file( GLOB Geometry Geometry/*.h Geometry/*.cpp )
file( GLOB GeometryBV Geometry/BV/*.h Geometry/BV/*.cpp )
source_group( "Geometry" FILES ${Geometry} )
source_group( "Geometry\\BV" FILES ${GeometryBV} )

#---------------------------------------------------------------------
# Image module
file( GLOB Image Image/*.h Image/*.cpp )
file( GLOB ImageSTB Image/stb/*.h )

source_group( "Image" FILES ${Image} )
source_group( "Image\\stb" FILES ${ImageSTB} )
#---------------------------------------------------------------------

#---------------------------------------------------------------------
# Render core module
file( GLOB RenderCore RenderCore/*.h RenderCore/*.cpp )
file( GLOB RenderCoreOpenGL45 RenderCore/OpenGL45/*.h RenderCore/OpenGL45/*.cpp )
file( GLOB RenderCoreOpenGL45GL RenderCore/OpenGL45/GL/*.h RenderCore/OpenGL45/GL/*.c )
file( GLOB RenderCoreFrameGraph RenderCore/FrameGraph/*.h RenderCore/FrameGraph/*.cpp )

source_group( "RenderCore" FILES ${RenderCore} )
source_group( "RenderCore\\OpenGL45" FILES ${RenderCoreOpenGL45} )
source_group( "RenderCore\\OpenGL45\\GL" FILES ${RenderCoreOpenGL45GL} )
source_group( "RenderCore\\FrameGraph" FILES ${RenderCoreFrameGraph} )

#---------------------------------------------------------------------
# Runtime module
file( GLOB Runtime Runtime/*.h Runtime/*.cpp )
file( GLOB RuntimeLWO Runtime/lwo/*.h Runtime/lwo/*.c )
file( GLOB RuntimeIESNA Runtime/iesna/*.h Runtime/iesna/*.c )
file( GLOB RuntimeSTB Runtime/stb/*.h Runtime/stb/*.c )

source_group( "Runtime" FILES ${Runtime} )
source_group( "Runtime\\lwo" FILES ${RuntimeLWO} )
source_group( "Runtime\\iesna" FILES ${RuntimeIESNA} )
source_group( "Runtime\\stb" FILES ${RuntimeSTB} )

#---------------------------------------------------------------------
# Audio module
file( GLOB Audio Audio/*.h Audio/*.cpp )

source_group( "Audio" FILES ${Audio} )

#---------------------------------------------------------------------
# Renderer module
file( GLOB Renderer Renderer/*.h Renderer/*.cpp )
file( GLOB RendererVT Renderer/VT/*.h Renderer/VT/*.cpp )

source_group( "Renderer" FILES ${Renderer} )
source_group( "Renderer\\VT" FILES ${RendererVT} )

#---------------------------------------------------------------------

add_executable( EmbedTool Tools/EmbedTool.cpp )
target_link_libraries( EmbedTool miniz )

message( ${CMAKE_CURRENT_BINARY_DIR} )

file(GLOB_RECURSE EmbeddedFiles Embedded/*)

add_custom_command(
OUTPUT EmbeddedResources.c
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Embedded
COMMAND EmbedTool ./ ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c
DEPENDS ${EmbeddedFiles}
)

source_group( "Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c )

add_library(

${PROJECT}

#SHARED

${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c

${Platform}
${PlatformMemory}

${Core}

${Containers}

${Geometry}
${GeometryBV}

${Image}
${ImageSTB}

${RenderCore}
${RenderCoreOpenGL45}
${RenderCoreOpenGL45GL}
${RenderCoreFrameGraph}

${Runtime}
${RuntimeLWO}
${RuntimeIESNA}
${RuntimeSTB}

${Audio}

${Renderer}
${RendererVT}
)

set (LIBRARIES
EASTL
fmt::fmt-header-only
${HK_BULLET_LIBS}
OpenGL::GL
HACD
vhacd
Recast
SDL2-static
muFFT
mimalloc-static
miniaudio
miniz
fastlz
glutess
polyclipping
tinyexr
cgltf
bc7enc
bcdec
lunasvg
${ANGELSCRIPT_LIBRARY_NAME}
angelscript_addon
)

if(UNIX)
    set( LIBRARIES ${LIBRARIES} uuid )
    set( LIBRARIES ${LIBRARIES} OpenGL::GLX )
endif()

target_link_libraries( ${PROJECT} ${LIBRARIES} )

set_target_properties( ${PROJECT}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../Binary/Win64"
)

target_compile_definitions(${PROJECT} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT} PUBLIC ${HK_COMPILER_FLAGS})
