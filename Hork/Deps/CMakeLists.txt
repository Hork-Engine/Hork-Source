project(Deps)

setup_msvc_runtime_library()

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

find_package(OpenGL REQUIRED)

file(GLOB_RECURSE EmbeddedFiles ../Embedded/*)
source_group("Embedded" FILES ${EmbeddedFiles})

add_custom_command(
	OUTPUT				EmbeddedResources.c
	WORKING_DIRECTORY	${CMAKE_CURRENT_SOURCE_DIR}/../Embedded
	COMMAND 			EmbedTool ./ ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c
	DEPENDS 			${EmbeddedFiles}
)

source_group("Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c)

add_library(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c)

target_link_libraries(${PROJECT_NAME}
    EASTL
    fmt::fmt-header-only
    HACD
    vhacd
    SDL3-static
    MikkTSpace
    mimalloc-static
    miniz
    fastlz
    glutess
    polyclipping
    tinyexr
    cgltf
    ufbx
    bc7enc
    bcdec
    lunasvg
    webp
    giflib
    OptickCore
    Jolt
    OpenGL::GL
    glslang::SPIRV
    glslang::glslang-default-resource-limits
    Recast
    muFFT
    miniaudio
    ozz_animation
    ozz_animation_offline
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} shcore)
endif()

if(UNIX)
    target_link_libraries(${PROJECT_NAME} OpenGL::GLX uuid)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${HK_COMPILER_FLAGS})

set_target_properties(${PROJECT_NAME}
	PROPERTIES
		OUTPUT_NAME 						"Hork-Deps"
		RUNTIME_OUTPUT_DIRECTORY 			"${CMAKE_BINARY_DIR}/../Binary/Win64"
)
