#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 3.7.2)

# Only generate Debug and Release configuration types.
set( CMAKE_CONFIGURATION_TYPES Debug Release )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${HK_PROJECT_BUILD_PATH} )
#set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${HK_PROJECT_BUILD_PATH} )
#set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${HK_PROJECT_BUILD_PATH} )
# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${HK_PROJECT_BUILD_PATH} )
#    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${HK_PROJECT_BUILD_PATH} )
#    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${HK_PROJECT_BUILD_PATH} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Use folders in the resulting project files.
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( CMAKE_POSITION_INDEPENDENT_CODE ON )

#list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS c)

# Setup Endianness
include( TestBigEndian )
TEST_BIG_ENDIAN( HK_IS_BIG_ENDIAN )
if ( HK_IS_BIG_ENDIAN )
add_definitions( -DHK_BIG_ENDIAN )
else()
add_definitions( -DHK_LITTLE_ENDIAN )
endif()


# Common Options
option(ENABLE_SHARED "Build shared libraries" OFF)
option(BUILD_SHARED_LIBS "Use shared libraries" OFF)

# Setup MSVC Runtime Library
if(MSVC)
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

# Copy a list of files from one directory to another. Relative files paths are maintained.
macro(HK_COPY_FILES target file_list source_dir target_dir)
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

# Compiler flags and defines
if(UNIX)
    set( HK_COMPILER_FLAGS
        -fvisibility=hidden
        -fno-exceptions                 # Disable exceptions
        -fno-builtin-memset
        -Werror                         # Treat warnings as errors
        -Wall                           # Enable all warnings
        -Wno-unused-parameter           # Don't warn about unused parameters
        -Wno-unused-function            # Don't warn about unused local function
        -Wno-sign-compare               # Don't warn about about mixed signed/unsigned type comparisons
        -Wno-strict-aliasing            # Don't warn about strict-aliasing rules
        -Wno-maybe-uninitialized
        -Wno-enum-compare
        )
    set( HK_COMPILER_DEFINES "" )
	
    # Don't generate thread-safe statics
    set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-threadsafe-statics )
endif()
if(WIN32)
    set(HK_COMPILER_FLAGS
        /W4           # Warning level 4
        /WX           # Treat warnings as errors
		/wd4018       # Ignore signed/unsigned mismatch
        /wd4100       # Ignore "unreferenced formal parameter" warning
        /wd4127       # Ignore "conditional expression is constant" warning
        /wd4201
		/wd4244       # Ignore "type conversion, possible loss of data"
		/wd4251       # Ignore "...needs to have dll-interface to be used by clients of class..."
		/wd4267       # Ignore "type conversion, possible loss of data"
        /wd4310
        /wd4324
		/wd4505       # Ignore "unused local function"
        /wd4592
        /wd4611
        /wd4714       # Ignore "force inline warning"
		/wd4996       # Ignore "deprecated functions"
        /Zc:threadSafeInit-  # Don't generate thread-safe statics
		/utf-8
		/FC           # __FILE__ contains full path
        )

    set( HK_COMPILER_DEFINES
        WIN32 _WIN32 _WINDOWS             # Windows platform
        UNICODE _UNICODE                  # Unicode build
        NOMINMAX                          # Use the standard's templated min/max
        WIN32_LEAN_AND_MEAN               # Exclude less common API declarations
        _HAS_EXCEPTIONS=0                 # Disable exceptions
        )
endif()

