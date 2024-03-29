project(Hork-Engine)

setup_msvc_runtime_library()

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

add_definitions(-DGLEW_STATIC)
add_definitions(-DGLEW_NO_GLU)

find_package(OpenGL REQUIRED)

make_source_list(SOURCE_FILES)

file(GLOB_RECURSE EmbeddedFiles Embedded/*)
source_group("Embedded" FILES ${EmbeddedFiles})

add_custom_command(
OUTPUT EmbeddedResources.c
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Embedded
COMMAND EmbedTool ./ ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c
DEPENDS ${EmbeddedFiles}
)

source_group("Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c)

add_library(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedResources.c ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME}
	EASTL
	fmt::fmt-header-only
	HACD
	vhacd
	SDL2-static
	MikkTSpace
	mimalloc-static
	miniz
	fastlz
	glutess
	polyclipping
	tinyexr
	cgltf
	fast_obj
	bc7enc
	bcdec
	lunasvg
	webp
	OptickCore
    BulletSoftBody
    BulletDynamics
    BulletCollision
    LinearMath
#    Bullet2FileLoader
#    BulletFileLoader
#    Bullet3Collision
#    Bullet3Common
#    Bullet3Dynamics
#    Bullet3Geometry
#    Bullet3OpenCL_clew
    BulletInverseDynamics
#    BulletInverseDynamicsUtils
#    BulletWorldImporter
#    BulletXmlWorldImporter
#    ConvexDecomposition
#    GIMPACTUtils
    OpenGL::GL
    Recast
    muFFT
    miniaudio
    ${ANGELSCRIPT_LIBRARY_NAME}
    angelscript_addon
)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} OpenGL::GLX uuid)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../Binary/Win64")

target_compile_definitions(${PROJECT_NAME} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${HK_COMPILER_FLAGS})
