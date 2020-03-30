set( PROJECT Engine )

project( ${PROJECT} )

setup_msvc_runtime_library()

add_definitions( -DANGIE_ENGINE_EXPORTS )
add_definitions( -DGLEW_STATIC )
add_definitions( -DGLEW_NO_GLU )


#########################################################################################
# Vulkan
message( "========== VULKAN ==========" )
find_package(Vulkan)
if (NOT Vulkan_FOUND)
    message(ERROR " Vulkan not found!")
endif(NOT Vulkan_FOUND)
include_directories( ${Vulkan_INCLUDE_DIRS} )
#########################################################################################

#########################################################################################
# SDL2
message( "========== SDL2 ==========" )
set(SDL_RENDER OFF)
add_subdirectory( ThirdParty/SDL2 )
set_target_properties( SDL2-static PROPERTIES FOLDER "ThirdParty" )
#########################################################################################

#########################################################################################
# NVIDIA TEXTURE TOOLS
#if ( AN_USE_NVTT )
#message( "========== NVIDIA TEXTURE TOOLS ==========" )
#option( NVTT_TOOLS "Nvtt Tools" OFF )
#option( NVTT_TESTS "Nvtt Tests" OFF )
#include_directories( ThirdParty/nvidia-texture-tools/src )
#include_directories( ThirdParty/nvidia-texture-tools/src/nvtt/squish )
#include_directories( ThirdParty/nvidia-texture-tools/extern/poshlib )
#include_directories( "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/nvidia-texture-tools/src" )
#add_subdirectory( ThirdParty/nvidia-texture-tools )
#set_target_properties( bc6h PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( bc7 PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( nvcore PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( nvimage PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( nvmath PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( nvthread PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( nvtt PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( posh PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set_target_properties( squish PROPERTIES FOLDER "ThirdParty/NvTextureTools" )
#set( NvTextureToolsLibs bc6h bc7 nvcore nvimage nvmath nvthread nvtt posh squish )
#set( AN_TOOLCHAIN_LIBS ${AN_TOOLCHAIN_LIBS} ${NvTextureToolsLibs} )
#endif()
#########################################################################################

#########################################################################################
# BULLET PHYSICS
message( "========== BULLET PHYSICS ==========" )
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

include_directories( ThirdParty/bullet3/src )

#set(BUILD_SHARED_LIBS ON)
add_subdirectory( ThirdParty/bullet3 )
#set(BUILD_SHARED_LIBS OFF)

set_target_properties( LinearMath PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletFileLoader PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( Bullet2FileLoader PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( Bullet3Collision PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( Bullet3Common PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( Bullet3Dynamics PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( Bullet3Geometry PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( Bullet3OpenCL_clew PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletCollision PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletDynamics PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletInverseDynamics PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletInverseDynamicsUtils PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletSoftBody PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletWorldImporter PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( BulletXmlWorldImporter PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( ConvexDecomposition PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( GIMPACTUtils PROPERTIES FOLDER "ThirdParty/Physics" )
set_target_properties( HACD PROPERTIES FOLDER "ThirdParty/Physics" )

set( AN_BULLET_LIBS LinearMath Bullet2FileLoader BulletFileLoader Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry Bullet3OpenCL_clew BulletCollision BulletDynamics BulletInverseDynamics BulletInverseDynamicsUtils BulletSoftBody BulletWorldImporter BulletXmlWorldImporter ConvexDecomposition GIMPACTUtils HACD )
#########################################################################################

#########################################################################################
# VHACD
message( "========== VHACD ==========" )
include_directories( ThirdParty/VHACD )
add_subdirectory( ThirdParty/VHACD )
include_directories( ThirdParty/VHACD/public )
set_target_properties( vhacd PROPERTIES FOLDER "ThirdParty/Physics" )
#########################################################################################

#########################################################################################
# BOX2D
#message( "========== BOX2D ==========" )
#option(BOX2D_INSTALL "Install Box2D libs, includes, and CMake scripts" OFF)
#option(BOX2D_INSTALL_DOC "Install Box2D documentation" OFF)
#option(BOX2D_BUILD_SHARED "Build Box2D shared libraries" OFF)
#option(BOX2D_BUILD_STATIC "Build Box2D static libraries" ON)
#option(BOX2D_BUILD_EXAMPLES "Build Box2D examples" OFF)
##include_directories( ThirdParty/Box2D/Box2D/Box2D )
#include_directories( ThirdParty/Box2D/Box2D )
#add_subdirectory( ThirdParty/Box2D/Box2D )
#set_target_properties(Box2D PROPERTIES FOLDER "ThirdParty/Physics")
#########################################################################################

#########################################################################################
# RECAST NAVIGATION
message( "========== RECAST NAVIGATION ==========" )
include_directories( ThirdParty/recastnavigation/DebugUtils/Include )
include_directories( ThirdParty/recastnavigation/Detour/Include )
include_directories( ThirdParty/recastnavigation/DetourCrowd/Include )
include_directories( ThirdParty/recastnavigation/DetourTileCache/Include )
include_directories( ThirdParty/recastnavigation/Recast/Include )
add_subdirectory( ThirdParty/recastnavigation )
set_target_properties( Recast PROPERTIES FOLDER "ThirdParty/Physics" )
#########################################################################################

#---------------------------------------------------------------------
# Core module
file( GLOB CorePublic Core/Public/*.h Core/Public/*.cpp )
file( GLOB CorePublicBV Core/Public/BV/*.h Core/Public/BV/*.cpp )
file( GLOB CorePrivate Core/Private/*.h Core/Private/*.cpp )
file( GLOB CorePrivateBV Core/Private/BV/*.h Core/Private/BV/*.cpp )
file( GLOB CorePrivateSTB Core/Private/stb/*.h Core/Private/stb/*.c )
file( GLOB CorePrivateFastLZ Core/Private/fastlz/*.h Core/Private/fastlz/*.c )
file( GLOB CorePrivateMiniZ Core/Private/miniz/*.h Core/Private/miniz/*.c )

source_group( "Core\\Public" FILES ${CorePublic} )
source_group( "Core\\Public\\BV" FILES ${CorePublicBV} )
source_group( "Core\\Private" FILES ${CorePrivate} )
source_group( "Core\\Private\\BV" FILES ${CorePrivateBV} )
source_group( "Core\\Private\\stb" FILES ${CorePrivateSTB} )
source_group( "Core\\Private\\fastlz" FILES ${CorePrivateFastLZ} )
source_group( "Core\\Private\\miniz" FILES ${CorePrivateMiniZ} )

#---------------------------------------------------------------------
# Runtime module
file( GLOB RuntimePublic Runtime/Public/*.h Runtime/Public/*.cpp )
file( GLOB RuntimePrivate Runtime/Private/*.h Runtime/Private/*.cpp )

source_group( "Runtime\\Public" FILES ${RuntimePublic} )
source_group( "Runtime\\Private" FILES ${RuntimePrivate} )

#---------------------------------------------------------------------
# OpenGL45 backend module
file( GLOB RendererOpenGL45 Renderer/OpenGL4.5/*.h Renderer/OpenGL4.5/*.cpp )
file( GLOB RendererOpenGL45GHI Renderer/OpenGL4.5/GHI/*.h Renderer/OpenGL4.5/GHI/*.cpp )
file( GLOB RendererOpenGL45GHIGL Renderer/OpenGL4.5/GHI/GL/*.h Renderer/OpenGL4.5/GHI/GL/*.c )

source_group( "Renderer\\OpenGL4.5" FILES ${RendererOpenGL45} )
source_group( "Renderer\\OpenGL4.5\\GHI" FILES ${RendererOpenGL45GHI} )
source_group( "Renderer\\OpenGL4.5\\GHI\\GL" FILES ${RendererOpenGL45GHIGL} )

#---------------------------------------------------------------------
# Vulkan backend module
file( GLOB RendererVulkan Renderer/Vulkan/*.h Renderer/Vulkan/*.cpp )

source_group( "Renderer\\Vulkan" FILES ${RendererVulkan} )

#---------------------------------------------------------------------
# Null-render backend module
file( GLOB RendererNull Renderer/Null/*.h Renderer/Null/*.cpp )

source_group( "Renderer\\Null" FILES ${RendererNull} )

#---------------------------------------------------------------------
# Imgui third party library
#file( GLOB Imgui imgui/*.h imgui/*.cpp )

#source_group( "imgui" FILES ${Imgui} )

#---------------------------------------------------------------------
# World public
file( GLOB WorldPublicBase World/Public/Base/*.h World/Public/Base/*.cpp )
file( GLOB WorldPublicResource World/Public/Resource/*.h World/Public/Resource/*.cpp )
file( GLOB WorldPublicMaterialGraph World/Public/MaterialGraph/*.h World/Public/MaterialGraph/*.cpp )
file( GLOB WorldPublicAudio World/Public/Audio/*.h World/Public/Audio/*.cpp )
file( GLOB WorldPublicAudioAudioCodec World/Public/Audio/AudioCodec/*.h World/Public/Audio/AudioCodec/*.cpp )
file( GLOB WorldPublicRender World/Public/Render/*.h World/Public/Render/*.cpp )
file( GLOB WorldPublicActors World/Public/Actors/*.h World/Public/Actors/*.cpp )
file( GLOB WorldPublicComponents World/Public/Components/*.h World/Public/Components/*.cpp )
file( GLOB WorldPublicWidgets World/Public/Widgets/*.h World/Public/Widgets/*.cpp )
file( GLOB WorldPublic World/Public/*.h World/Public/*.cpp )

source_group( "World\\Public\\Base" FILES ${WorldPublicBase} )
source_group( "World\\Public\\Resource" FILES ${WorldPublicResource} )
source_group( "World\\Public\\MaterialGraph" FILES ${WorldPublicMaterialGraph} )
source_group( "World\\Public\\Audio" FILES ${WorldPublicAudio} )
source_group( "World\\Public\\Audio\\AudioCodec" FILES ${WorldPublicAudioAudioCodec} )
source_group( "World\\Public\\Render" FILES ${WorldPublicRender} )
source_group( "World\\Public\\Actors" FILES ${WorldPublicActors} )
source_group( "World\\Public\\Components" FILES ${WorldPublicComponents} )
source_group( "World\\Public\\Widgets" FILES ${WorldPublicWidgets} )
source_group( "World\\Public" FILES ${WorldPublic} )

#---------------------------------------------------------------------
# World private
file( GLOB WorldPrivateBase World/Private/Base/*.h World/Private/Base/*.cpp )
file( GLOB WorldPrivateResource World/Private/Resource/*.h World/Private/Resource/*.cpp )
file( GLOB WorldPrivateResourceGLTF World/Private/Resource/gltf/*.h )
file( GLOB WorldPrivateResourceLWO World/Private/Resource/lwo/*.h World/Private/Resource/lwo/*.c )
file( GLOB WorldPrivateMaterialGraph World/Private/MaterialGraph/*.h World/Private/MaterialGraph/*.cpp )
file( GLOB WorldPrivateAudio World/Private/Audio/*.h World/Private/Audio/*.cpp World/Private/Audio/*.c )
file( GLOB WorldPrivateAudioAudioCodec World/Private/Audio/AudioCodec/*.h World/Private/Audio/AudioCodec/*.cpp )
file( GLOB WorldPrivateAudioAudioCodecLibmpg123 World/Private/Audio/AudioCodec/libmpg123/*.h )
file( GLOB WorldPrivateAudioAudioCodecSTB World/Private/Audio/AudioCodec/stb/*.h )
file( GLOB WorldPrivateAudioAL World/Private/Audio/AL/*.h )
file( GLOB WorldPrivateRender World/Private/Render/*.h World/Private/Render/*.cpp )
file( GLOB WorldPrivateActors World/Private/Actors/*.h World/Private/Actors/*.cpp )
file( GLOB WorldPrivateComponents World/Private/Components/*.h World/Private/Components/*.cpp )
file( GLOB WorldPrivateWidgets World/Private/Widgets/*.h World/Private/Widgets/*.cpp )
file( GLOB WorldPrivateBulletCompatibility World/Private/BulletCompatibility/*.h World/Private/BulletCompatibility/*.cpp )
file( GLOB WorldPrivate World/Private/*.h World/Private/*.cpp )

source_group( "World\\Private\\Base" FILES ${WorldPrivateBase} )
source_group( "World\\Private\\Resource" FILES ${WorldPrivateResource} )
source_group( "World\\Private\\Resource\\gltf" FILES ${WorldPrivateResourceGLTF} )
source_group( "World\\Private\\Resource\\lwo" FILES ${WorldPrivateResourceLWO} )
source_group( "World\\Private\\MaterialGraph" FILES ${WorldPrivateMaterialGraph} )
source_group( "World\\Private\\Audio" FILES ${WorldPrivateAudio} )
source_group( "World\\Private\\Audio\\AudioCodec" FILES ${WorldPrivateAudioAudioCodec} )
source_group( "World\\Private\\Audio\\AudioCodec\\libmpg123" FILES ${WorldPrivateAudioAudioCodecLibmpg123} )
source_group( "World\\Private\\Audio\\AudioCodec\\stb" FILES ${WorldPrivateAudioAudioCodecSTB} )
source_group( "World\\Private\\Audio\\AL" FILES ${WorldPrivateAudioAL} )
source_group( "World\\Private\\Render" FILES ${WorldPrivateRender} )
source_group( "World\\Private\\Actors" FILES ${WorldPrivateActors} )
source_group( "World\\Private\\Components" FILES ${WorldPrivateComponents} )
source_group( "World\\Private\\Widgets" FILES ${WorldPrivateWidgets} )
source_group( "World\\Private\\BulletCompatibility" FILES ${WorldPrivateBulletCompatibility} )
source_group( "World\\Private" FILES ${WorldPrivate} )

#---------------------------------------------------------------------
# Game thread module
file( GLOB GameThreadPublic GameThread/Public/*.h GameThread/Public/*.cpp )
file( GLOB GameThreadPrivate GameThread/Private/*.h GameThread/Private/*.cpp )

source_group( "GameThread\\Public" FILES ${GameThreadPublic} )
source_group( "GameThread\\Private" FILES ${GameThreadPrivate} )

#---------------------------------------------------------------------

add_library(

${PROJECT}

#SHARED

${CorePublic}
${CorePublicBV}
${CorePrivate}
${CorePrivateBV}
${CorePrivateSTB}
${CorePrivateFastLZ}
${CorePrivateMiniZ}

${RuntimePublic}
${RuntimePrivate}

${RendererOpenGL45}
${RendererOpenGL45GHI}
${RendererOpenGL45GHIGL}
#${RendererVulkan}
${RendererNull}

${Imgui}

${WorldPublicBase}
${WorldPublicResource}
${WorldPublicMaterialGraph}
${WorldPublicAudio}
${WorldPublicAudioAudioCodec}
${WorldPublicRender}
${WorldPublicActors}
${WorldPublicComponents}
${WorldPublicWidgets}
${WorldPublic}
${WorldPrivateBase}
${WorldPrivateResource}
${WorldPrivateResourceGLTF}
${WorldPrivateResourceLWO}
${WorldPrivateMaterialGraph}
${WorldPrivateAudio}
${WorldPrivateAudioAudioCodec}
${WorldPrivateAudioAudioCodecLibmpg123}
${WorldPrivateAudioAudioCodecSTB}
${WorldPrivateAudioAL}
${WorldPrivateRender}
${WorldPrivateActors}
${WorldPrivateComponents}
${WorldPrivateWidgets}
${WorldPrivateBulletCompatibility}
${WorldPrivate}

${GameThreadPublic}
${GameThreadPrivate}
)

# if shared
set_property( TARGET ${PROJECT} PROPERTY POSITION_INDEPENDENT_CODE ON )

set( LIBRARIES
        ${AN_BULLET_LIBS}
        ${OPENGL_gl_LIBRARY}
        ${Vulkan_LIBRARIES}
        vhacd
        Recast
        SDL2-static
#    poly2tri Clipper glutess Box2D
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

