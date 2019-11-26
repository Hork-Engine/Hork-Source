
set( PROJECT Engine )

project( ${PROJECT} )

setup_msvc_runtime_library()

add_definitions( -DANGIE_ENGINE_EXPORTS )

#---------------------------------------------------------------------
# Core module
file( GLOB CorePublic Core/Public/*.h Core/Public/*.cpp )
file( GLOB CorePublicBV Core/Public/BV/*.h Core/Public/BV/*.cpp )
file( GLOB CorePrivate Core/Private/*.h Core/Private/*.cpp )
file( GLOB CorePrivateBV Core/Private/BV/*.h Core/Private/BV/*.cpp )

source_group( "Core\\Public" FILES ${CorePublic} )
source_group( "Core\\Public\\BV" FILES ${CorePublicBV} )
source_group( "Core\\Private" FILES ${CorePrivate} )
source_group( "Core\\Private\\BV" FILES ${CorePrivateBV} )

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

source_group( "Renderer\\OpenGL4.5" FILES ${RendererOpenGL45} )
source_group( "Renderer\\OpenGL4.5\\GHI" FILES ${RendererOpenGL45GHI} )

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
file( GLOB Imgui imgui/*.h imgui/*.cpp )

source_group( "imgui" FILES ${Imgui} )

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
file( GLOB WorldPrivateMaterialGraph World/Private/MaterialGraph/*.h World/Private/MaterialGraph/*.cpp )
file( GLOB WorldPrivateAudio World/Private/Audio/*.h World/Private/Audio/*.cpp )
file( GLOB WorldPrivateAudioAudioCodec World/Private/Audio/AudioCodec/*.h World/Private/Audio/AudioCodec/*.cpp )
file( GLOB WorldPrivateRender World/Private/Render/*.h World/Private/Render/*.cpp )
file( GLOB WorldPrivateActors World/Private/Actors/*.h World/Private/Actors/*.cpp )
file( GLOB WorldPrivateComponents World/Private/Components/*.h World/Private/Components/*.cpp )
file( GLOB WorldPrivateWidgets World/Private/Widgets/*.h World/Private/Widgets/*.cpp )
file( GLOB WorldPrivateBulletCompatibility World/Private/BulletCompatibility/*.h World/Private/BulletCompatibility/*.cpp )
file( GLOB WorldPrivate World/Private/*.h World/Private/*.cpp )

source_group( "World\\Private\\Base" FILES ${WorldPrivateBase} )
source_group( "World\\Private\\Resource" FILES ${WorldPrivateResource} )
source_group( "World\\Private\\MaterialGraph" FILES ${WorldPrivateMaterialGraph} )
source_group( "World\\Private\\Audio" FILES ${WorldPrivateAudio} )
source_group( "World\\Private\\Audio\\AudioCodec" FILES ${WorldPrivateAudioAudioCodec} )
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

${RuntimePublic}
${RuntimePrivate}

${RendererOpenGL45}
${RendererOpenGL45GHI}
${RendererVulkan}
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
${WorldPrivateMaterialGraph}
${WorldPrivateAudio}
${WorldPrivateAudioAudioCodec}
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

#target_link_libraries( ${PROJECT} ${AN_BULLET_LIBS} Box2D Recast  fastlz ${ZLIB_LIBRARY} minizip libpng jpeg-static poly2tri Clipper glutess )

if(WIN32)
set( AN_OS_LIBS )
else()
set( AN_OS_LIBS uuid )
endif()

target_link_libraries( ${PROJECT} ${AN_BULLET_LIBS} ${AN_OPENGL_LIBS} ${Vulkan_LIBRARIES} ${AN_OS_LIBS} ${ZLIB_LIBRARY} minizip fastlz vhacd Recast )

set_target_properties( ${PROJECT}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../Binary/Win64"
)

target_compile_definitions(${PROJECT} PUBLIC ${AN_COMPILER_DEFINES})
target_compile_options(${PROJECT} PUBLIC ${AN_COMPILER_FLAGS})

