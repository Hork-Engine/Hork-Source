
set( PROJECT Engine )

project( ${PROJECT} )

setup_msvc_runtime_library()

add_definitions( -DANGIE_ENGINE_EXPORTS )

#file( GLOB Root *.h *.cpp )

file( GLOB CorePublic Core/Public/*.h Core/Public/*.cpp )
file( GLOB CorePublicBV Core/Public/BV/*.h Core/Public/BV/*.cpp )
file( GLOB CorePrivate Core/Private/*.h Core/Private/*.cpp )
file( GLOB CorePrivateBV Core/Private/BV/*.h Core/Private/BV/*.cpp )

file( GLOB RuntimePublic Runtime/Public/*.h Runtime/Public/*.cpp )
file( GLOB RuntimePrivate Runtime/Private/*.h Runtime/Private/*.cpp )

file( GLOB RendererOpenGL45 Renderer/OpenGL4.5/*.h Renderer/OpenGL4.5/*.cpp )
file( GLOB RendererOpenGL45GHI Renderer/OpenGL4.5/GHI/*.h Renderer/OpenGL4.5/GHI/*.cpp )

file( GLOB RendererVulkan Renderer/Vulkan/*.h Renderer/Vulkan/*.cpp )

file( GLOB RendererNull Renderer/Null/*.h Renderer/Null/*.cpp )

file( GLOB Imgui imgui/*.h imgui/*.cpp )

file( GLOB BulletCompatibility BulletCompatibility/*.h BulletCompatibility/*.cpp )

file( GLOB BasePublic Base/Public/*.h Base/Public/*.cpp )
file( GLOB BasePrivate Base/Private/*.h Base/Private/*.cpp )

file( GLOB ResourcePublic Resource/Public/*.h Resource/Public/*.cpp )
file( GLOB ResourcePrivate Resource/Private/*.h Resource/Private/*.cpp )

file( GLOB AudioPublic Audio/Public/*.h Audio/Public/*.cpp )
file( GLOB AudioPrivate Audio/Private/*.h Audio/Private/*.cpp )

file( GLOB AudioPublicAudioCodec Audio/Public/AudioCodec/*.h Audio/Public/AudioCodec/*.cpp )
file( GLOB AudioPrivateAudioCodec Audio/Private/AudioCodec/*.h Audio/Private/AudioCodec/*.cpp )

file( GLOB WorldPublic World/Public/*.h World/Public/*.cpp )
file( GLOB WorldPrivate World/Private/*.h World/Private/*.cpp )

file( GLOB WorldPublicActors World/Public/Actors/*.h World/Public/Actors/*.cpp )
file( GLOB WorldPrivateActors World/Private/Actors/*.h World/Private/Actors/*.cpp )

file( GLOB WorldPublicComponents World/Public/Components/*.h World/Public/Components/*.cpp )
file( GLOB WorldPrivateComponents World/Private/Components/*.h World/Private/Components/*.cpp )

file( GLOB GameThreadPublic GameThread/Public/*.h GameThread/Public/*.cpp )
file( GLOB GameThreadPrivate GameThread/Private/*.h GameThread/Private/*.cpp )

source_group( "Core\\Public" FILES ${CorePublic} )
source_group( "Core\\Public\\BV" FILES ${CorePublicBV} )
source_group( "Core\\Private" FILES ${CorePrivate} )
source_group( "Core\\Private\\BV" FILES ${CorePrivateBV} )

source_group( "Runtime\\Public" FILES ${RuntimePublic} )
source_group( "Runtime\\Private" FILES ${RuntimePrivate} )

source_group( "Renderer\\OpenGL4.5" FILES ${RendererOpenGL45} )
source_group( "Renderer\\OpenGL4.5\\GHI" FILES ${RendererOpenGL45GHI} )

source_group( "Renderer\\Vulkan" FILES ${RendererVulkan} )

source_group( "Renderer\\Null" FILES ${RendererNull} )

source_group( "imgui" FILES ${Imgui} )

source_group( "Base\\Public" FILES ${BasePublic} )
source_group( "Base\\Private" FILES ${BasePrivate} )

source_group( "BulletCompatibility" FILES ${BulletCompatibility} )

source_group( "Resource\\Public" FILES ${ResourcePublic} )
source_group( "Resource\\Private" FILES ${ResourcePrivate} )

source_group( "Audio\\Public" FILES ${AudioPublic} )
source_group( "Audio\\Private" FILES ${AudioPrivate} )

source_group( "Audio\\Public\\AudioCodec" FILES ${AudioPublicAudioCodec} )
source_group( "Audio\\Private\\AudioCodec" FILES ${AudioPrivateAudioCodec} )

source_group( "World\\Public" FILES ${WorldPublic} )
source_group( "World\\Private" FILES ${WorldPrivate} )

source_group( "World\\Public\\Actors" FILES ${WorldPublicActors} )
source_group( "World\\Private\\Actors" FILES ${WorldPrivateActors} )

source_group( "World\\Public\\Components" FILES ${WorldPublicComponents} )
source_group( "World\\Private\\Components" FILES ${WorldPrivateComponents} )

source_group( "GameThread\\Public" FILES ${GameThreadPublic} )
source_group( "GameThread\\Private" FILES ${GameThreadPrivate} )

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

${BasePublic}
${BasePrivate}

${BulletCompatibility}

${ResourcePublic}
${ResourcePrivate}

${AudioPublic}
${AudioPrivate}

${AudioPublicAudioCodec}
${AudioPrivateAudioCodec}

${WorldPublic}
${WorldPrivate}

${WorldPublicActors}
${WorldPrivateActors}

${WorldPublicComponents}
${WorldPrivateComponents}

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

