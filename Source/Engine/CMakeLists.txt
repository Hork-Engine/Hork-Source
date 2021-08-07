set( PROJECT AngieEngine )

project( ${PROJECT} )

setup_msvc_runtime_library()

add_definitions( -DANGIE_ENGINE_EXPORTS )


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

set( AN_BULLET_LIBS
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
# DiligentCore
#
#include_directories(${PROJECT} PRIVATE "DiligentCore")
#add_subdirectory_with_folder("ThirdParty/DiligentCore" ThirdParty/DiligentCore)


#---------------------------------------------------------------------
# Core module
file( GLOB CorePublic Core/Public/*.h Core/Public/*.cpp )
file( GLOB CorePublicBV Core/Public/BV/*.h Core/Public/BV/*.cpp )
file( GLOB CorePrivate Core/Private/*.h Core/Private/*.cpp )
file( GLOB CorePrivateBV Core/Private/BV/*.h Core/Private/BV/*.cpp )
file( GLOB CorePrivateSTB Core/Private/stb/*.h )

source_group( "Core\\Public" FILES ${CorePublic} )
source_group( "Core\\Public\\BV" FILES ${CorePublicBV} )
source_group( "Core\\Private" FILES ${CorePrivate} )
source_group( "Core\\Private\\BV" FILES ${CorePrivateBV} )
source_group( "Core\\Private\\stb" FILES ${CorePrivateSTB} )

#---------------------------------------------------------------------
# Render core module
file( GLOB RenderCore RenderCore/*.h RenderCore/*.cpp )
file( GLOB RenderCoreOpenGL45 RenderCore/OpenGL45/*.h RenderCore/OpenGL45/*.cpp )
file( GLOB RenderCoreOpenGL45GL RenderCore/OpenGL45/GL/*.h RenderCore/OpenGL45/GL/*.c )
#file( GLOB RenderCoreDiligent RenderCore/Diligent/*.h RenderCore/Diligent/*.cpp )
file( GLOB RenderCoreFrameGraph RenderCore/FrameGraph/*.h RenderCore/FrameGraph/*.cpp )

source_group( "RenderCore" FILES ${RenderCore} )
source_group( "RenderCore\\OpenGL45" FILES ${RenderCoreOpenGL45} )
source_group( "RenderCore\\OpenGL45\\GL" FILES ${RenderCoreOpenGL45GL} )
#source_group( "RenderCore\\Diligent" FILES ${RenderCoreDiligent} )
source_group( "RenderCore\\FrameGraph" FILES ${RenderCoreFrameGraph} )

#---------------------------------------------------------------------
# Runtime module
file( GLOB RuntimePublic Runtime/Public/*.h Runtime/Public/*.cpp )
file( GLOB RuntimePrivate Runtime/Private/*.h Runtime/Private/*.cpp )

source_group( "Runtime\\Public" FILES ${RuntimePublic} )
source_group( "Runtime\\Private" FILES ${RuntimePrivate} )

#---------------------------------------------------------------------
# Audio module
file( GLOB AudioPublic Audio/Public/*.h Audio/Public/*.cpp )
file( GLOB AudioPrivate Audio/Private/*.h Audio/Private/*.cpp )

source_group( "Audio\\Public" FILES ${AudioPublic} )
source_group( "Audio\\Private" FILES ${AudioPrivate} )

#---------------------------------------------------------------------
# Renderer module
file( GLOB Renderer Renderer/*.h Renderer/*.cpp )
file( GLOB RendererVT Renderer/VT/*.h Renderer/VT/*.cpp )

source_group( "Renderer" FILES ${Renderer} )
source_group( "Renderer\\VT" FILES ${RendererVT} )

#---------------------------------------------------------------------
# World public
file( GLOB WorldPublicBase World/Public/Base/*.h World/Public/Base/*.cpp )
file( GLOB WorldPublicResource World/Public/Resource/*.h World/Public/Resource/*.cpp )
file( GLOB WorldPublicMaterialGraph World/Public/MaterialGraph/*.h World/Public/MaterialGraph/*.cpp )
file( GLOB WorldPublicRender World/Public/Render/*.h World/Public/Render/*.cpp )
file( GLOB WorldPublicActors World/Public/Actors/*.h World/Public/Actors/*.cpp )
file( GLOB WorldPublicComponents World/Public/Components/*.h World/Public/Components/*.cpp )
file( GLOB WorldPublicWidgets World/Public/Widgets/*.h World/Public/Widgets/*.cpp )
file( GLOB WorldPublic World/Public/*.h World/Public/*.cpp )

source_group( "World\\Public\\Base" FILES ${WorldPublicBase} )
source_group( "World\\Public\\Resource" FILES ${WorldPublicResource} )
source_group( "World\\Public\\MaterialGraph" FILES ${WorldPublicMaterialGraph} )
source_group( "World\\Public\\Render" FILES ${WorldPublicRender} )
source_group( "World\\Public\\Actors" FILES ${WorldPublicActors} )
source_group( "World\\Public\\Components" FILES ${WorldPublicComponents} )
source_group( "World\\Public\\Widgets" FILES ${WorldPublicWidgets} )
source_group( "World\\Public" FILES ${WorldPublic} )

#---------------------------------------------------------------------
# World private
file( GLOB WorldPrivateBase World/Private/Base/*.h World/Private/Base/*.cpp )
file( GLOB WorldPrivateResource World/Private/Resource/*.h World/Private/Resource/*.cpp )
file( GLOB WorldPrivateResourceLWO World/Private/Resource/lwo/*.h World/Private/Resource/lwo/*.c )
file( GLOB WorldPrivateResourceIESNA World/Private/Resource/iesna/*.h World/Private/Resource/iesna/*.c )
file( GLOB WorldPrivateMaterialGraph World/Private/MaterialGraph/*.h World/Private/MaterialGraph/*.cpp )
file( GLOB WorldPrivateRender World/Private/Render/*.h World/Private/Render/*.cpp )
file( GLOB WorldPrivateActors World/Private/Actors/*.h World/Private/Actors/*.cpp )
file( GLOB WorldPrivateComponents World/Private/Components/*.h World/Private/Components/*.cpp )
file( GLOB WorldPrivateWidgets World/Private/Widgets/*.h World/Private/Widgets/*.cpp )
file( GLOB WorldPrivateBulletCompatibility World/Private/BulletCompatibility/*.h World/Private/BulletCompatibility/*.cpp )
file( GLOB WorldPrivate World/Private/*.h World/Private/*.cpp )

source_group( "World\\Private\\Base" FILES ${WorldPrivateBase} )
source_group( "World\\Private\\Resource" FILES ${WorldPrivateResource} )
source_group( "World\\Private\\Resource\\lwo" FILES ${WorldPrivateResourceLWO} )
source_group( "World\\Private\\Resource\\iesna" FILES ${WorldPrivateResourceIESNA} )
source_group( "World\\Private\\MaterialGraph" FILES ${WorldPrivateMaterialGraph} )
source_group( "World\\Private\\Render" FILES ${WorldPrivateRender} )
source_group( "World\\Private\\Actors" FILES ${WorldPrivateActors} )
source_group( "World\\Private\\Components" FILES ${WorldPrivateComponents} )
source_group( "World\\Private\\Widgets" FILES ${WorldPrivateWidgets} )
source_group( "World\\Private\\BulletCompatibility" FILES ${WorldPrivateBulletCompatibility} )
source_group( "World\\Private" FILES ${WorldPrivate} )

#---------------------------------------------------------------------

add_executable( EmbedTool Util/EmbedTool.cpp )
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

${CorePublic}
${CorePublicBV}
${CorePrivate}
${CorePrivateBV}
${CorePrivateSTB}

${RenderCore}
${RenderCoreOpenGL45}
${RenderCoreOpenGL45GL}
#${RenderCoreDiligent}
${RenderCoreFrameGraph}

${RuntimePublic}
${RuntimePrivate}

${AudioPublic}
${AudioPrivate}

${Renderer}
${RendererVT}

${Imgui}

${WorldPublicBase}
${WorldPublicResource}
${WorldPublicMaterialGraph}
${WorldPublicRender}
${WorldPublicActors}
${WorldPublicComponents}
${WorldPublicWidgets}
${WorldPublic}
${WorldPrivateBase}
${WorldPrivateResource}
${WorldPrivateResourceLWO}
${WorldPrivateResourceIESNA}
${WorldPrivateMaterialGraph}
${WorldPrivateRender}
${WorldPrivateActors}
${WorldPrivateComponents}
${WorldPrivateWidgets}
${WorldPrivateBulletCompatibility}
${WorldPrivate}
)

# if shared
set_property( TARGET ${PROJECT} PROPERTY POSITION_INDEPENDENT_CODE ON )

set( LIBRARIES
        ${AN_BULLET_LIBS}
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

#    Diligent-GraphicsEngineD3D11-static
#    Diligent-GraphicsEngineOpenGL-static
#    Diligent-GraphicsEngineD3D12-static
#    Diligent-GraphicsEngineVk-static
)

if(UNIX)
    set( LIBRARIES ${LIBRARIES} uuid )
endif()

target_link_libraries( ${PROJECT} ${LIBRARIES} )

set_target_properties( ${PROJECT}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../Binary/Win64"
)

target_compile_definitions(${PROJECT} PUBLIC ${AN_COMPILER_DEFINES})
target_compile_options(${PROJECT} PUBLIC ${AN_COMPILER_FLAGS})
