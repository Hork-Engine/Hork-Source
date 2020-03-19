project(VHACD_LIB CXX C)
#include(${CMAKE_COMMON_INC})

setup_msvc_runtime_library()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if (NOT NO_OPENCL)
	#include(FindOpenCL OPTIONAL)
	find_package(OpenCL)
endif()
#if (NOT NO_OPENMP)
#	#include(FindOpenMP OPTIONAL)
#	find_package(OpenMP)
#endif()
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
#set(LIB_TYPE "STATIC" CACHE STRING "STATIC, SHARED or MODULE?")
#message("[VHACD] \t LIB_TYPE " ${LIB_TYPE})

set( PROJECT_SOURCES
    src/btAlignedAllocator.cpp
    src/btConvexHullComputer.cpp
    src/FloatMath.cpp
    src/FloatMath.inl
    src/VHACD.cpp
    src/VHACD-ASYNC.cpp
    src/vhacdICHull.cpp
    src/vhacdManifoldMesh.cpp
    src/vhacdMesh.cpp
    src/vhacdRaycastMesh.cpp
    src/vhacdVolume.cpp
    )

set( PROJECT_HEADERS
    inc/btAlignedAllocator.h
    inc/btAlignedObjectArray.h
    inc/btConvexHullComputer.h
    inc/btMinMax.h
    inc/btScalar.h
    inc/btVector3.h
    inc/FloatMath.h
    inc/vhacdCircularList.h
    inc/vhacdCircularList.inl
    inc/vhacdICHull.h
    inc/vhacdManifoldMesh.h
    inc/vhacdMesh.h
    inc/vhacdMutex.h
    inc/vhacdRaycastMesh.h
    inc/vhacdSArray.h
    inc/vhacdTimer.h
    inc/vhacdVector.h
    inc/vhacdVector.inl
    inc/vhacdVHACD.h
    inc/vhacdVolume.h
    )

set( PROJECT_CL_FILES
    cl/vhacdKernels.cl
    )

add_library(vhacd ${LIB_TYPE} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_CL_FILES})

if (OpenCL_FOUND)
    target_include_directories(vhacd PRIVATE "${OpenCL_INCLUDE_DIR}")
    target_link_libraries(vhacd PRIVATE "${OpenCL_LIBRARY}")
    target_compile_definitions(vhacd PRIVATE -DOPENCL_FOUND=1 )
    target_compile_definitions(vhacd PRIVATE -DOPENCL_CL_FILES="${PROJECT_CL_FILES}" )
endif()

#if (OPENMP_FOUND)
#    target_link_libraries(vhacd PRIVATE "${OPENMP_LIBRARY}")
#    target_link_libraries(vhacd PRIVATE OpenMP::OpenMP_CXX)
#endif()

target_include_directories(vhacd PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
#  $<INSTALL_INTERFACE:include/>  # <prefix>/include/mylib
  )

#target_include_directories(vhacd PUBLIC
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
#  $<INSTALL_INTERFACE:include/>  # <prefix>/include/mylib
#  )


#message("[VHACD] \t -> CMAKE_INSTALL_PREFIX " ${CMAKE_INSTALL_PREFIX})
#install(TARGETS vhacd EXPORT vhacd-targets DESTINATION lib)
#install(FILES ${PROJECT_INC_FILES} DESTINATION include)
#install(FILES ${PROJECT_INL_FILES} DESTINATION include)


#set(VHACD_LIB_VERSION 3.2.0)
#include(CMakePackageConfigHelpers)
#write_basic_package_version_file(
#  "${CMAKE_CURRENT_BINARY_DIR}/vhacd/vhacd-config-version.cmake"
#  VERSION ${VHACD_LIB_VERSION}
#  COMPATIBILITY AnyNewerVersion
#)

#export(EXPORT vhacd-targets NAMESPACE ::
#  FILE "${CMAKE_CURRENT_BINARY_DIR}/vhacd/vhacd-targets.cmake"
#)

#configure_file(cmake/vhacd-config.cmake
#  "${CMAKE_CURRENT_BINARY_DIR}/vhacd/vhacd-config.cmake"
#  COPYONLY
#)

#set(ConfigPackageLocation lib/cmake/vhacd)
#install(EXPORT vhacd-targets
#  FILE
#    vhacd-targets.cmake
#  DESTINATION
#    ${ConfigPackageLocation}
#	NAMESPACE ::
#)
#install(
#  FILES
#    "${CMAKE_CURRENT_BINARY_DIR}/vhacd/vhacd-config.cmake"
#    "${CMAKE_CURRENT_BINARY_DIR}/vhacd/vhacd-config-version.cmake"
#  DESTINATION
#    ${ConfigPackageLocation}
#  COMPONENT
#    Devel
#)
