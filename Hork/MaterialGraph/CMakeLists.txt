project(MaterialGraph)

setup_msvc_runtime_library()

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

make_source_list(SOURCE_FILES)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} Renderer)

target_compile_definitions(${PROJECT_NAME} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${HK_COMPILER_FLAGS})

set_target_properties(${PROJECT_NAME}
	PROPERTIES
		OUTPUT_NAME 						"Hork-MaterialGraph"
		RUNTIME_OUTPUT_DIRECTORY 			"${CMAKE_BINARY_DIR}/../Binary/Win64"
)
