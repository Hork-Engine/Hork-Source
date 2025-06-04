project(12_Terrain)

setup_msvc_runtime_library()

make_source_list(SOURCE_FILES)
make_source_list_for_directory(../Source COMMON_FILES)

file(GLOB RESOURCES res/resource.rc res/hork.ico)
source_group("res" FILES ${RESOURCES})

set(SOURCE_FILES ${SOURCE_FILES} ${COMMON_FILES} ${RESOURCES})

if(WIN32)
add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
else()
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
endif()

target_link_libraries(${PROJECT_NAME} Runtime)

target_compile_definitions(${PROJECT_NAME} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${HK_COMPILER_FLAGS})
