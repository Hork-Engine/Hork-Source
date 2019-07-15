
set( PROJECT Engine )

project( ${PROJECT} )

setup_msvc_runtime_library()

add_definitions( -DANGIE_ENGINE_EXPORTS )

#include_directories( Audio/Private/Codec/ThirdParty )

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

file( GLOB GameEnginePublic GameEngine/Public/*.h GameEngine/Public/*.cpp )
file( GLOB GameEnginePrivate GameEngine/Private/*.h GameEngine/Private/*.cpp )

file( GLOB GameEnginePrivateBulletCompatibility GameEngine/Private/BulletCompatibility/*.h GameEngine/Private/BulletCompatibility/*.cpp )

file( GLOB GameEnginePublicCodec GameEngine/Public/Codec/*.h GameEngine/Public/Codec/*.cpp )
file( GLOB GameEnginePrivateCodec GameEngine/Private/Codec/*.h GameEngine/Private/Codec/*.cpp )

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

source_group( "GameEngine\\Public" FILES ${GameEnginePublic} )
source_group( "GameEngine\\Private" FILES ${GameEnginePrivate} )

source_group( "GameEngine\\Private\\BulletCompatibility" FILES ${GameEnginePrivateBulletCompatibility} )

source_group( "GameEngine\\Public\\Codec" FILES ${GameEnginePublicCodec} )
source_group( "GameEngine\\Private\\Codec" FILES ${GameEnginePrivateCodec} )

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

${GameEnginePublic}
${GameEnginePrivate}

${GameEnginePrivateBulletCompatibility}

${GameEnginePublicCodec}
${GameEnginePrivateCodec}

${GameEnginePublicMesh}
${GameEnginePrivateMesh}

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

