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
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()

include_directories( ThirdParty )

#
# GLEW
#
add_definitions( -DGLEW_STATIC )
add_definitions( -DGLEW_NO_GLU )

#
# SDL2
#
set(SDL_RENDER OFF)
add_subdirectory_with_folder( "ThirdParty/SDL" ThirdParty/SDL2 )

#
# Bullet Physics
#
option( USE_DOUBLE_PRECISION "Use double precision" OFF )
option( USE_GRAPHICAL_BENCHMARK "Use Graphical Benchmark" ON )
option( USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD "Use btSoftMultiBodyDynamicsWorld" OFF )
option( BULLET2_USE_THREAD_LOCKS "Build Bullet 2 libraries with mutex locking around certain operations" OFF )
option( USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF )
option( USE_CUSTOM_VECTOR_MATH "Use custom vectormath library" OFF )
option( BUILD_CPU_DEMOS "Build original Bullet CPU examples" OFF )
option( USE_GLUT "Use Glut" OFF )
option( BUILD_OPENGL3_DEMOS "Set when you want to build the OpenGL3+ demos" OFF )
option( BUILD_BULLET2_DEMOS "Set when you want to build the Bullet 2 demos" OFF )
option( BUILD_UNIT_TESTS "Build Unit Tests" OFF )
option( BUILD_BULLET3 "Set when you want to build Bullet 3" OFF)

include_directories( ThirdParty/bullet3/src )
add_subdirectory_with_folder( "ThirdParty/BulletPhysics" ThirdParty/bullet3 )

set( HK_BULLET_LIBS
BulletSoftBody
BulletDynamics
BulletCollision
LinearMath
#Bullet2FileLoader
#BulletFileLoader
#Bullet3Collision
Bullet3Common
#Bullet3Dynamics
#Bullet3Geometry
#Bullet3OpenCL_clew
BulletInverseDynamics
BulletInverseDynamicsUtils
BulletWorldImporter
#BulletXmlWorldImporter
ConvexDecomposition
GIMPACTUtils
HACD )

#
# VHACD
#
include_directories( ThirdParty/VHACD )
include_directories( ThirdParty/VHACD/public )
add_subdirectory_with_folder( "ThirdParty/VHACD" ThirdParty/VHACD )

#
# Recast&Detour
#
include_directories( ThirdParty/recastnavigation/DebugUtils/Include )
include_directories( ThirdParty/recastnavigation/Detour/Include )
include_directories( ThirdParty/recastnavigation/DetourCrowd/Include )
include_directories( ThirdParty/recastnavigation/DetourTileCache/Include )
include_directories( ThirdParty/recastnavigation/Recast/Include )
add_subdirectory_with_folder( "ThirdParty/recastnavigation" ThirdParty/recastnavigation )

#
# muFFT
#
add_subdirectory_with_folder( "ThirdParty/muFFT" ThirdParty/muFFT )

#
# miniaudio
#
add_subdirectory_with_folder( "ThirdParty/miniaudio" ThirdParty/miniaudio )

#
# miniz
#
add_subdirectory_with_folder( "ThirdParty/miniz" ThirdParty/miniz )

#
# fastlz
#
add_subdirectory_with_folder( "ThirdParty/fastlz" ThirdParty/fastlz )

#
# glutess
#
add_subdirectory_with_folder( "ThirdParty/glutess" ThirdParty/glutess )

#
# clipper
#
add_subdirectory_with_folder( "ThirdParty/clipper" ThirdParty/clipper )

#
# tinyexr
#
add_subdirectory_with_folder( "ThirdParty/tinyexr" ThirdParty/tinyexr )

#
# cgltf
#
add_subdirectory_with_folder( "ThirdParty/cgltf" ThirdParty/cgltf )

#
# AngelScript
#
#add_subdirectory_with_folder( "ThirdParty/angelscript" ThirdParty/angelscript )

add_subdirectory(ThirdParty/angelscript/projects/cmake angelscript)
add_subdirectory(ThirdParty/angelscript_addon angelscript_addon)
include_directories(ThirdParty/angelscript/include)
include_directories(ThirdParty/angelscript_addon)
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
file( GLOB Core Core/*.h Core/*.cpp )
file( GLOB CoreSTB Core/stb/*.h )

source_group( "Core" FILES ${Core} )
source_group( "Core\\stb" FILES ${CoreSTB} )

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
${CoreSTB}

${Containers}

${Geometry}
${GeometryBV}

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

${Imgui}
)

# if shared
set_property( TARGET ${PROJECT} PROPERTY POSITION_INDEPENDENT_CODE ON )

set( LIBRARIES
        ${HK_BULLET_LIBS}
        ${OPENGL_gl_LIBRARY}
        vhacd
        Recast
        SDL2-static
        muFFT
		miniaudio
		miniz
		fastlz
        glutess
        polyclipping
        tinyexr
        cgltf
		${ANGELSCRIPT_LIBRARY_NAME}
		angelscript_addon
)

if(UNIX)
    set( LIBRARIES ${LIBRARIES} uuid )
endif()

target_link_libraries( ${PROJECT} ${LIBRARIES} )

set_target_properties( ${PROJECT}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../Binary/Win64"
)

target_compile_definitions(${PROJECT} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT} PUBLIC ${HK_COMPILER_FLAGS})
