#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 3.7.2)

# Only generate Debug and Release configuration types.
set( CMAKE_CONFIGURATION_TYPES Debug Release )

# Project name.
project( AngieEngine )

set( PROJECT_OUTPUT_DIR "E:/AngieBinary" )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR} )
#set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR} )
#set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR} )
# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_OUTPUT_DIR} )
#    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_OUTPUT_DIR} )
#    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_OUTPUT_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Use folders in the resulting project files.
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# C++11 standatd
if ( CMAKE_COMPILER_IS_GNUCC )
add_definitions( "-std=c++11" )
endif()

set( CMAKE_POSITION_INDEPENDENT_CODE ON )

#########################################################################################
# Setup Endianness

include( TestBigEndian )
TEST_BIG_ENDIAN( AN_IS_BIG_ENDIAN )
if ( AN_IS_BIG_ENDIAN )
add_definitions( -DAN_BIG_ENDIAN )
else()
add_definitions( -DAN_LITTLE_ENDIAN )
endif()

#########################################################################################

#########################################################################################
# Common Options

option(ENABLE_SHARED "Build shared libraries" OFF)
option(BUILD_SHARED_LIBS "Use shared libraries" OFF)

option(AN_USE_WEB_BROWSER "Compile with CEF" OFF)

option(AN_USE_NVTT, "Use NVidia Texture Tools" OFF )

set( AN_USE_NVTT 1 )

if ( AN_USE_WEB_BROWSER )
add_definitions( -DAN_WEB_BROWSER )
endif()

#########################################################################################

#########################################################################################
# Setup MSVC Runtime Library

if (MSVC)
    option(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC runtime library DLL" OFF)
endif()

macro(setup_msvc_runtime_library)
if (MSVC)
    if (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
        foreach (flag CMAKE_C_FLAGS
                      CMAKE_C_FLAGS_DEBUG
                      CMAKE_C_FLAGS_RELEASE
                      CMAKE_C_FLAGS_MINSIZEREL
                      CMAKE_C_FLAGS_RELWITHDEBINFO
					  CMAKE_CXX_FLAGS
					  CMAKE_CXX_FLAGS_DEBUG
					  CMAKE_CXX_FLAGS_RELEASE
					  CMAKE_CXX_FLAGS_MINSIZEREL
					  CMAKE_CXX_FLAGS_RELWITHDEBINFO)

            if (${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
            if (${flag} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
            endif()
        endforeach()
    endif()
endif()
endmacro()
#########################################################################################

#########################################################################################
# Copy a list of files from one directory to another. Relative files paths are maintained.
macro(AN_COPY_FILES target file_list source_dir target_dir)
  foreach(FILENAME ${file_list})
    set(source_file ${source_dir}/${FILENAME})
    set(target_file ${target_dir}/${FILENAME})
    if(IS_DIRECTORY ${source_file})
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${source_file}" "${target_file}"
        VERBATIM
        )
    else()
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${source_file}" "${target_file}"
        VERBATIM
        )
    endif()
  endforeach()
endmacro()
#########################################################################################

if(UNIX)
  set(AN_COMPILER_FLAGS
    -fvisibility=hidden
    -Werror                         # Treat warnings as errors
    -Wall                           # Enable all warnings
    -Wno-unused-parameter           # Don't warn about unused parameters
    -fno-exceptions                 # Disable exceptions
    -fno-threadsafe-statics         # Don't generate thread-safe statics
    -fno-builtin-memset
    -Wsign-compare                  # Warn about mixed signed/unsigned type comparisons
  )
  set(AN_COMPILER_DEFINES "")
endif()

#########################################################################################
# Compiler flags and defines
if(WIN32)
  set(AN_COMPILER_FLAGS
     /W4           # Warning level 4
	 /WX           # Treat warnings as errors
	 /wd4100       # Ignore "unreferenced formal parameter" warning
	 /wd4127       # Ignore "conditional expression is constant" warning
	 /wd4201
	 /wd4310
	 /wd4324
	 /wd4592
	 /wd4611
	 /wd4714       # Ignore force inline warning
	 )

  set(AN_COMPILER_DEFINES
    WIN32 _WIN32 _WINDOWS             # Windows platform
    UNICODE _UNICODE                  # Unicode build
    NOMINMAX                          # Use the standard's templated min/max
    WIN32_LEAN_AND_MEAN               # Exclude less common API declarations
    _HAS_EXCEPTIONS=0                 # Disable exceptions
    )
endif()

#########################################################################################
# CEF setup.
if ( AN_USE_WEB_BROWSER )
message( "========== CEF ==========" )
if (UNIX)
set( CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/cef/linux" )
else()
set( CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/cef/win" )
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake" )
# Execute FindCEF.cmake which must exist in CMAKE_MODULE_PATH.
find_package(CEF REQUIRED)
# Include the libcef_dll_wrapper target.
# Comes from the libcef_dll/CMakeLists.txt file in the binary distribution
# directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
set_target_properties(libcef_dll_wrapper PROPERTIES FOLDER "ThirdParty")
include_directories( ${CEF_ROOT} )
# Display configuration settings.
#PRINT_CEF_CONFIG()
endif()
#########################################################################################

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
# GLFW
message( "========== GLFW ==========" )
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_VULKAN_STATIC "Use the Vulkan loader statically linked into application" ON)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
include_directories( ThirdParty/glfw/include )
add_definitions( -DAN_GLFW_EXTENSION )
add_subdirectory( ThirdParty/glfw )
set_target_properties( glfw PROPERTIES FOLDER "ThirdParty" )
set( AN_OPENGL_LIBS ${AN_OPENGL_LIBS} glfw )
#########################################################################################

#########################################################################################
# GLEW
message( "========== GLEW ==========" )
find_package( OpenGL REQUIRED )
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)
add_definitions( -DGLEW_STATIC )
add_definitions( -DGLEW_NO_GLU )
#add_definitions( -DGLEW_MX )
include_directories( ThirdParty/glew/include )
add_subdirectory( ThirdParty/glew )
set_target_properties( glew PROPERTIES FOLDER "ThirdParty" )
set( AN_OPENGL_LIBS ${AN_OPENGL_LIBS} ${OPENGL_gl_LIBRARY} glew )
#link_libraries( glew )
#########################################################################################

#########################################################################################
# GLM
#message( "========== GLM ==========" )
#include_directories( ThirdParty/glm )
#add_subdirectory( ThirdParty/glm )
#set_target_properties( glm_dummy PROPERTIES FOLDER "ThirdParty" )
#########################################################################################

#########################################################################################
# OpenAL
include_directories( ThirdParty )
#########################################################################################

#########################################################################################
# RAPID JSON
#message( "========== RAPID JSON ==========" )
#option(RAPIDJSON_BUILD_DOC "Build rapidjson documentation." OFF)
#option(RAPIDJSON_BUILD_EXAMPLES "Build rapidjson examples." OFF)
#option(RAPIDJSON_BUILD_TESTS "Build rapidjson perftests and unittests." OFF)
#option(RAPIDJSON_BUILD_THIRDPARTY_GTEST "Use gtest installation in `thirdparty/gtest` by default if available" OFF)
#option(RAPIDJSON_BUILD_CXX11 "Build rapidjson with C++11 (gcc/clang)" ON)
#option(RAPIDJSON_BUILD_ASAN "Build rapidjson with address sanitizer (gcc/clang)" OFF)
#option(RAPIDJSON_BUILD_UBSAN "Build rapidjson with undefined behavior sanitizer (gcc/clang)" OFF)
#option(RAPIDJSON_HAS_STDSTRING "" OFF)
#include_directories( ThirdParty/rapidjson/include )
#add_subdirectory( ThirdParty/rapidjson )
#########################################################################################

#########################################################################################
# STB
message( "========== STB ==========" )
include_directories( ThirdParty/stb )
#########################################################################################

#########################################################################################
# POSHLIB
message( "========== POSHLIB ==========" )
include_directories( ThirdParty/poshlib )
add_subdirectory( ThirdParty/poshlib )
set_target_properties( posh PROPERTIES FOLDER "ThirdParty" )
#link_libraries( glfw )
#########################################################################################

#########################################################################################
# ZLIB & MINIZIP
message( "========== ZLIB ==========" )
set( ZLIB_INCLUDE_DIR ThirdParty/zlib )
set( ZLIB_LIBRARY zlibstatic )
include_directories( ThirdParty/zlib )
include_directories( "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/zlib" )
include_directories( ThirdParty/minizip )
add_subdirectory( ThirdParty/zlib )
add_subdirectory( ThirdParty/minizip )
set_target_properties( zlibstatic PROPERTIES FOLDER "ThirdParty/Compression" )
set_target_properties( minizip PROPERTIES FOLDER "ThirdParty/Compression" )
#########################################################################################

#########################################################################################
# FASTLZ
message( "========== FASTLZ ==========" )
include_directories( ThirdParty/fastlz )
add_subdirectory( ThirdParty/fastlz )
set_target_properties( fastlz PROPERTIES FOLDER "ThirdParty/Compression" )
#set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} fastlz )
#########################################################################################

#########################################################################################
# JPEG-TURBO
#message( "========== JPEG-TURBO ==========" )
##if (UNIX)
##Fixme
#option(WITH_SIMD "Include SIMD extensions" OFF)
##endif()
#set( JPEG_INCLUDE_DIR ThirdParty/libjpeg-turbo )
#include_directories( ${JPEG_INCLUDE_DIR} )
#include_directories( "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/libjpeg-turbo" )
#add_definitions( -DSIZEOF_SIZE_T=${CMAKE_SIZEOF_VOID_P} )
#add_subdirectory( ThirdParty/libjpeg-turbo )
#set_target_properties( jpeg-static PROPERTIES FOLDER "ThirdParty/Image" )
#set_target_properties( turbojpeg-static PROPERTIES FOLDER "ThirdParty/Image" )
##set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} jpeg-static turbojpeg-static )
#########################################################################################

#########################################################################################
# PNG
#message( "========== PNG ==========" )
#include_directories( ThirdParty/libpng )
#include_directories( "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/libpng" )
#add_subdirectory( ThirdParty/libpng )
#set_target_properties( libpng PROPERTIES FOLDER "ThirdParty/Image" )
##set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} libpng )
#########################################################################################

#########################################################################################
# JASPER
#if ( AN_USE_JASPER )
#message( "========== JASPER ==========" )
#option(JAS_ENABLE_SHARED "Enable building of shared library" OFF)
#option(JAS_ENABLE_LIBJPEG "Enable the use of the JPEG Library" ON)
#option(JAS_ENABLE_OPENGL "Enable the use of the OpenGL/GLUT Library" OFF)
#option(JAS_ENABLE_STRICT "Enable pedantic error checking" OFF)
#option(JAS_ENABLE_AUTOMATIC_DEPENDENCIES "Enable automatic dependencies" ON)
#option(JAS_LOCAL "Enable local hacks for developers (do not enable)" OFF)
#include_directories( ThirdParty/jasper/src/libjasper/include )
#include_directories( "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/jasper/src/libjasper/include" )
#add_definitions(
# -DJAS_GEO_OMIT_PRINTING_CODE
# -DEXCLUDE_JPG_SUPPORT
# -DEXCLUDE_MIF_SUPPORT
# -DEXCLUDE_PNM_SUPPORT
# -DEXCLUDE_RAS_SUPPORT
# -DEXCLUDE_BMP_SUPPORT
# -DEXCLUDE_PGX_SUPPORT
# -DEXCLUDE_TIFF_SUPPORT
#)
##add_subdirectory( ThirdParty/jasper/src/libjasper )
#add_subdirectory( ThirdParty/jasper )
#set_target_properties( libjasper PROPERTIES FOLDER "ThirdParty/Image" )
#set( AN_TOOLCHAIN_LIBS ${AN_TOOLCHAIN_LIBS} libjasper )
#endif()
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
# DEV-IL
#if ( AN_USE_DEVIL )
#message( "========== DEV-IL ==========" )
#option( IL_USE_DXTC_SQUISH "Use libsquish for DXTC support" OFF )
#include_directories( ThirdParty/DevIL/DevIL/include )
##include_directories( ThirdParty/DevIL/DevIL/src-IL/include )
##include_directories( ThirdParty/DevIL/DevIL/src-ILU/include )
#add_subdirectory( ThirdParty/DevIL/DevIL )
#set_target_properties( IL PROPERTIES FOLDER "ThirdParty/Image")
#set( AN_TOOLCHAIN_LIBS ${AN_TOOLCHAIN_LIBS} IL )
#add_definitions( -DIL_STATIC_LIB )
#endif()
#########################################################################################

#########################################################################################
# CLIPPER
#message( "========== CLIPPER ==========" )
#add_subdirectory( ThirdParty/Clipper )
#set_target_properties( Clipper PROPERTIES FOLDER "ThirdParty" )
#########################################################################################

#########################################################################################
# POLY2TRI
#message( "========== POLY2TRI ==========" )
#add_subdirectory( ThirdParty/poly2tri )
#set_target_properties( poly2tri PROPERTIES FOLDER "ThirdParty" )
#########################################################################################

#########################################################################################
# GLUTESS
#message( "========== GLUTESS ==========" )
#add_subdirectory( ThirdParty/glutess )
#set_target_properties( glutess PROPERTIES FOLDER "ThirdParty" )
#########################################################################################

#########################################################################################
# LIBMPG123
#message( "========== LIBMPG123 ==========" )
#include_directories( ThirdParty/mpg123-1.25.6 )
#include_directories( ThirdParty/mpg123-1.25.6/compat )
#include_directories( ThirdParty/mpg123-1.25.6/libmpg )
#add_subdirectory( ThirdParty/mpg123-1.25.6 )
#set_target_properties( libmpg123 PROPERTIES FOLDER "ThirdParty" )
#########################################################################################

#########################################################################################
# IMGUI
#message( "========== IMGUI ==========" )
#include_directories( ThirdParty/imgui )
#add_subdirectory( ThirdParty/imgui )
#set_target_properties( imgui PROPERTIES FOLDER "ThirdParty")
#set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} imgui )
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
#set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} fastlz )
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
##set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} Box2D )
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
#set( AN_THIRDPARTY_LIBS ${AN_THIRDPARTY_LIBS} Recast )
#########################################################################################

#########################################################################################
# Engine
message( "========== ENGINE ==========" )
include_directories( ThirdParty/.. )
include_directories( Engine )
add_definitions( -DANGIE_STATIC_LIBRARY )
add_subdirectory( Engine )
#########################################################################################

#########################################################################################
# Toolchain
message( "========== TOOLCHAIN ==========" )
#add_subdirectory( Toolchain )
##set_target_properties( Toolchain PROPERTIES FOLDER "Toolchain" )

# New kind of toolchain
#add_subdirectory( Editor2 )

#########################################################################################

#########################################################################################
# SAMPLES
message( "========== SAMPLES ==========" )
#add_subdirectory( Samples/CharacterController )
#set_target_properties( CharacterController PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/Quake1 )
set_target_properties( Quake1 PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/AssimpConverter )
#set_target_properties( AssimpConverter PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/Portals )
set_target_properties( Portals PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/Sponza )
set_target_properties( Sponza PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/Physics )
set_target_properties( Physics PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/Shooter3D )
set_target_properties( Shooter3D PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/WitchhammerRPG )
set_target_properties( WitchhammerRPG PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/TestGame )
#set_target_properties( TestGame PROPERTIES FOLDER "Samples" )
add_subdirectory( Samples/Airplane )
set_target_properties( Airplane PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/ClusteredTest )
#set_target_properties( ClusteredTest PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/SkinnedView )
#set_target_properties( SkinnedView PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/PBRTest )
#set_target_properties( PBRTest PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/sRGBTest )
#set_target_properties( sRGBTest PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/PhysicsTest )
#set_target_properties( PhysicsTest PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/WebBrowser )
#set_target_properties( WebBrowser PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/DLLTest )
#set_target_properties( DLLTest PROPERTIES FOLDER "Samples" )
#add_subdirectory( Samples/GameDLL )
#set_target_properties( GameDLL PROPERTIES FOLDER "Samples" )
#########################################################################################

#########################################################################################
# ANGIE GAME
#message( "========== ANGIE GAME ==========" )
#add_subdirectory( AngieGame )
#set_target_properties( AngieGame PROPERTIES FOLDER "Games" )
#########################################################################################

#########################################################################################
# BLADE OF DARKNESS GAME
#message( "========== BLADE OF DARKNESS GAME ==========" )
#add_subdirectory( BladeOfDarkness/trunk )
#set_target_properties( BladeOfDarkness PROPERTIES FOLDER "Games" )
#########################################################################################

# Set StartUp project
set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT TestGame )
