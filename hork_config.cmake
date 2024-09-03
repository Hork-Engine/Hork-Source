#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 3.16.3)

# Only generate Debug and Release configuration types.
set( CMAKE_CONFIGURATION_TYPES Debug Release )

set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED HK_PROJECT_BUILD_PATH)
    set(HK_PROJECT_BUILD_PATH ${CMAKE_BINARY_DIR}/Build)
endif()

file(MAKE_DIRECTORY ${HK_PROJECT_BUILD_PATH})

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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
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

# Compiler flags
if(MSVC)
    set(HK_COMPILER_FLAGS
        /W4                 # Warning level 4
        /WX                 # Treat warnings as errors
        /wd4018             # Ignore signed/unsigned mismatch
        /wd4100             # Ignore "unreferenced formal parameter" warning
        /wd4127             # Ignore "conditional expression is constant" warning
        /wd4201
        /wd4244             # Ignore "type conversion, possible loss of data"
        /wd4251             # Ignore "...needs to have dll-interface to be used by clients of class..."
        /wd4267             # Ignore "type conversion, possible loss of data"
        /wd4310
        /wd4324
        /wd4505             # Ignore "unused local function"
        /wd4592
        /wd4611
        /wd4714             # Ignore "force inline warning"
        /wd4996             # Ignore "deprecated functions"
		/wd26812            # Suppress C++ code analysis warning C26812
        /Zc:threadSafeInit  # Thread-safe statics
        /utf-8
        /FC                 # __FILE__ contains full path
    )
else()
    set(HK_COMPILER_FLAGS
        -fvisibility=hidden
        -fno-exceptions                 # Disable exceptions
#       -Werror                         # Treat warnings as errors
        -Wall                           # Enable all warnings
        -Wno-unused-parameter           # Don't warn about unused parameters
        -Wno-unused-function            # Don't warn about unused local function
        -Wno-sign-compare               # Don't warn about about mixed signed/unsigned type comparisons
        -Wno-strict-aliasing            # Don't warn about strict-aliasing rules
        -Wno-maybe-uninitialized
        -Wno-enum-compare
        -Wno-unused-local-typedefs
        -Wno-unused-value
        -Wno-switch
        -Wno-deprecated-declarations
        )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder")
endif()

# Compiler defines
if(UNIX)
    set(HK_COMPILER_DEFINES "")
endif()

if(WIN32)
    set(HK_COMPILER_DEFINES
        WIN32
        _WIN32
        _WINDOWS             # Windows platform
        UNICODE
        _UNICODE             # Unicode build
        NOMINMAX             # Use the standard's templated min/max
        WIN32_LEAN_AND_MEAN  # Exclude less common API declarations
        _HAS_EXCEPTIONS=0    # Disable exceptions
    )
endif()

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        get_target_property(_type ${_target} TYPE)
        if (NOT ${_type} STREQUAL "INTERFACE_LIBRARY")
            set_target_properties(${_target} PROPERTIES FOLDER "${_folder_name}")
        endif()
    endforeach()
endfunction()

# Recursive scan current source directory and group by folders. Store result in SRC.
function(make_source_list SRC)
	file(GLOB_RECURSE SOURCE_LIST CONFIGURE_DEPENDS "*.h" "*.hpp" "*.c" "*.cpp" "*.inl")

	foreach(FILE ${SOURCE_LIST}) 
	  get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

	  file(RELATIVE_PATH RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PARENT_DIR})

	  string(REPLACE "/" "\\" GROUP "${RELATIVE}")

	  source_group("${GROUP}" FILES "${FILE}")

	  #message("GROUP: " ${GROUP} " FILE: "${FILE})
	endforeach()
	set(${SRC} ${SOURCE_LIST} PARENT_SCOPE)
endfunction()

# Recursive scan specified source directory and group by folders. Store result in SRC.
function(make_source_list_for_directory DIR SRC)
	file(GLOB_RECURSE SOURCE_LIST CONFIGURE_DEPENDS "${DIR}/*.h" "${DIR}/*.hpp" "${DIR}/*.c" "${DIR}/*.cpp"  "${DIR}/*.inl")
	
	set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${DIR})

	foreach(FILE ${SOURCE_LIST}) 
	  get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

	  file(RELATIVE_PATH RELATIVE ${BASE_DIR} ${PARENT_DIR})

	  string(REPLACE "/" "\\" GROUP "${RELATIVE}")

	  source_group("${GROUP}" FILES "${FILE}")

	  #message("GROUP: " ${GROUP} " FILE: " ${FILE})
	endforeach()
	set(${SRC} ${SOURCE_LIST} PARENT_SCOPE)
endfunction()

function(make_link SOURCE DESTINATION)
	if(WIN32)
	  file(TO_NATIVE_PATH "${DESTINATION}" DESTINATION_NATIVE)
	  file(TO_NATIVE_PATH "${SOURCE}" SOURCE_NATIVE)	  
	  if (NOT EXISTS ${DESTINATION_NATIVE})
		execute_process(
		  COMMAND cmd /c mklink /J "${DESTINATION_NATIVE}" "${SOURCE_NATIVE}"
		  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
		)
	  endif()
	else()
	  if (NOT EXISTS ${DESTINATION})
		execute_process(COMMAND ln -s ${SOURCE} ${DESTINATION})
	  endif()
	endif()
endfunction()

# Macro from Urho source code
# Macro for setting symbolic link on platform that supports it
macro (create_symlink SOURCE DESTINATION)
    cmake_parse_arguments (ARG "FALLBACK_TO_COPY" "BASE" "" ${ARGN})
    # Make absolute paths so they work more reliably on cmake-gui
    if (IS_ABSOLUTE ${SOURCE})
        set (ABS_SOURCE ${SOURCE})
    else ()
        set (ABS_SOURCE ${CMAKE_SOURCE_DIR}/${SOURCE})
    endif ()
    if (IS_ABSOLUTE ${DESTINATION})
        set (ABS_DESTINATION ${DESTINATION})
    else ()
        if (ARG_BASE)
            set (ABS_DESTINATION ${ARG_BASE}/${DESTINATION})
        else ()
            set (ABS_DESTINATION ${CMAKE_BINARY_DIR}/${DESTINATION})
        endif ()
    endif ()
    if (CMAKE_HOST_WIN32)
        if (IS_DIRECTORY ${ABS_SOURCE})
            set (SLASH_D /D)
        else ()
            unset (SLASH_D)
        endif ()
        if (HAS_MKLINK)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_D} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET)
            endif ()
        elseif (ARG_FALLBACK_TO_COPY)
            if (SLASH_D)
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory ${ABS_SOURCE} ${ABS_DESTINATION})
            else ()
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_SOURCE} ${ABS_DESTINATION})
            endif ()
            # Fallback to copy only one time
            execute_process (${COMMAND})
            if (TARGET ${TARGET_NAME})
                # Fallback to copy every time the target is built
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD ${COMMAND})
            endif ()
        else ()
            message (WARNING "Unable to create symbolic link on this host system, you may need to manually copy file/dir from \"${SOURCE}\" to \"${DESTINATION}\"")
        endif ()
    else ()
        get_filename_component (DIRECTORY ${ABS_DESTINATION} DIRECTORY)
        file (RELATIVE_PATH REL_SOURCE ${DIRECTORY} ${ABS_SOURCE})
        execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink ${REL_SOURCE} ${ABS_DESTINATION})
    endif ()
endmacro ()

# Macro from Urho source code
# Macro for setting up header files installation for the SDK and the build tree (only support subset of install command arguments)
#  FILES <list> - File list to be installed
#  DIRECTORY <list> - Directory list to be installed
#  FILES_MATCHING - Option to perform file pattern matching on DIRECTORY list
#  PATTERN <list> - Pattern list to be used in file pattern matching option
#  DESTINATION <value> - A relative destination path to be installed to
macro (install_header_files)
    # Need to check if the destination variable is defined first because this macro could be called by downstream project that does not wish to install anything
    #if (HK_DEST_INCLUDE_DIR)
        # Parse the arguments for the underlying install command for the SDK
        cmake_parse_arguments (ARG "FILES_MATCHING" "DESTINATION" "FILES;DIRECTORY;PATTERN" ${ARGN})
        unset (INSTALL_MATCHING)
        if (ARG_FILES)
            set (INSTALL_TYPE FILES)
            set (INSTALL_SOURCES ${ARG_FILES})
        elseif (ARG_DIRECTORY)
            set (INSTALL_TYPE DIRECTORY)
            set (INSTALL_SOURCES ${ARG_DIRECTORY})
            if (ARG_FILES_MATCHING)
                set (INSTALL_MATCHING FILES_MATCHING)
                # Our macro supports PATTERN <list> but CMake's install command does not, so convert the list to: PATTERN <value1> PATTERN <value2> ...
                foreach (PATTERN ${ARG_PATTERN})
                    list (APPEND INSTALL_MATCHING PATTERN ${PATTERN})
                endforeach ()
            endif ()
        else ()
            message (FATAL_ERROR "Couldn't setup install command because the install type is not specified.")
        endif ()
        if (NOT ARG_DESTINATION)
            message (FATAL_ERROR "Couldn't setup install command because the install destination is not specified.")
        endif ()

        install (${INSTALL_TYPE} ${INSTALL_SOURCES} DESTINATION ${ARG_DESTINATION} ${INSTALL_MATCHING})

        # Reparse the arguments for the create_symlink macro to "install" the header files in the build tree

        foreach (INSTALL_SOURCE ${INSTALL_SOURCES})
            if (NOT IS_ABSOLUTE ${INSTALL_SOURCE})
                set (INSTALL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${INSTALL_SOURCE})
            endif ()
            if (INSTALL_SOURCE MATCHES /$)
                    # Use file symlink for each individual files in the source directory
                    if (IS_SYMLINK ${ARG_DESTINATION} AND NOT CMAKE_HOST_WIN32)
                        execute_process (COMMAND ${CMAKE_COMMAND} -E remove ${ARG_DESTINATION})
                    endif ()
                    set (GLOBBING_EXPRESSION RELATIVE ${INSTALL_SOURCE})
                    if (ARG_FILES_MATCHING)
                        foreach (PATTERN ${ARG_PATTERN})
                            list (APPEND GLOBBING_EXPRESSION ${INSTALL_SOURCE}${PATTERN})
                        endforeach ()
                    else ()
                        list (APPEND GLOBBING_EXPRESSION ${INSTALL_SOURCE}*)
                    endif ()
                    file (GLOB_RECURSE NAMES ${GLOBBING_EXPRESSION})
                    foreach (NAME ${NAMES})
                        get_filename_component (PATH ${ARG_DESTINATION}/${NAME} PATH)
                        # Recreate the source directory structure in the destination path
                        if (NOT EXISTS ${CMAKE_BINARY_DIR}/${PATH})
                            file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${PATH})
                        endif ()
                        create_symlink (${INSTALL_SOURCE}${NAME} ${ARG_DESTINATION}/${NAME} BASE ${CMAKE_BINARY_DIR} FALLBACK_TO_COPY)
                    endforeach ()
            else ()
                # Source is a file (it could also be actually a directory to be treated as a "file", i.e. for creating symlink pointing to the directory)
                get_filename_component (NAME ${INSTALL_SOURCE} NAME)
                create_symlink (${INSTALL_SOURCE} ${ARG_DESTINATION}/${NAME} BASE ${CMAKE_BINARY_DIR} FALLBACK_TO_COPY)
            endif ()
        endforeach ()
    #endif ()
endmacro ()



function(install_thirdparty_includes SOURCE_PATH DESTINATION_PATH)
	set(header_files *.h *.hpp *.inl)
	install_header_files(DIRECTORY ${SOURCE_PATH}/ DESTINATION include/ThirdParty/${DESTINATION_PATH} FILES_MATCHING PATTERN ${header_files})
endfunction()
