
FindGitRevision()
add_definitions(-DGIT_REVISION=\"${GIT_REVISION}\")

if (DEFINED VISUS_HOME)
 	add_definitions(-DVISUS_HOME="${VISUS_HOME}")
endif()

if (DEFINED VISUS_PYTHON_SYS_PATH)
 	add_definitions(-DVISUS_PYTHON_SYS_PATH="${VISUS_PYTHON_SYS_PATH}")
endif()

if (NOT (WIN32 OR APPLE))

  include(CheckIncludeFiles)
  unset(HAS_UUID_H)
  check_include_files(uuid/uuid.h HAS_UUID_H)

  if (HAS_UUID_H)
    add_definitions(-DHAS_UUID_H=1)
   else()
    add_definitions(-DHAS_UUID_H=0)
  endif()
endif()


set(UpLevelSources
	./include/Visus/Visus.h 
	./include/Visus/Kernel.h 
	./src/Visus.cpp 
	./src/Kernel.cpp)

IF (APPLE OR WIN32)
	set(UpLevelSources ${UpLevelSources} ./src/Kernel.mm)
endif()

set(CoreSources
	./include/Visus/Aborted.h
	./include/Visus/Async.h
	./include/Visus/BigInt.h
	./include/Visus/Color.h
	./include/Visus/CriticalSection.h
	./include/Visus/Diff.h
	./include/Visus/DirectoryIterator.h
	./include/Visus/Exception.h
	./include/Visus/File.h
	./include/Visus/HeapMemory.h
	./include/Visus/ScopedVector.h
	./include/Visus/Log.h
	./include/Visus/Model.h
	./include/Visus/NumericLimits.h
	./include/Visus/Object.h
	./include/Visus/Path.h
	./include/Visus/Platform.h
	./include/Visus/RamResource.h
	./include/Visus/Semaphore.h
	./include/Visus/Singleton.h
	./include/Visus/SignalSlot.h
	./include/Visus/StringMap.h
	./include/Visus/StringTree.h
	./include/Visus/StringUtils.h
	./include/Visus/Time.h
	./include/Visus/Thread.h
	./include/Visus/ThreadPool.h
	./include/Visus/Url.h
	./include/Visus/Utils.h
	./include/Visus/UUID.h
	./src/Color.cpp
	./src/CriticalSection.cpp
	./src/Diff.cpp
	./src/DirectoryIterator.cpp
	./src/Exception.cpp
	./src/File.cpp
	./src/HeapMemory.cpp
	./src/Log.cpp
	./src/Object.cpp
	./src/Path.cpp
	./src/RamResource.cpp
	./src/Semaphore.cpp
	./src/StringMap.cpp
	./src/StringTree.cpp
	./src/StringUtils.cpp
	./src/Time.cpp
	./src/Thread.cpp 
	./src/ThreadPool.cpp 
	./src/Url.cpp
	./src/Utils.cpp
	./src/UUID.cpp
	./include/Visus/ApplicationStats.h
	./src/ApplicationStats.cpp
	./include/Visus/ApplicationInfo.h
	./src/ApplicationInfo.cpp
	./include/Visus/VisusConfig.h
	./src/VisusConfig.cpp)

IF (WIN32 OR APPLE)
	set(CoreSources ${CoreSources} ./src/DirectoryIterator.mm)
endif()

set(GeometrySources
	./include/Visus/Matrix.h 
	./include/Visus/Box.h
	./include/Visus/Circle.h
	./include/Visus/Frustum.h
	./include/Visus/Line.h
	./include/Visus/LinearMap.h
	./include/Visus/LocalCoordinateSystem.h
	./include/Visus/Plane.h
	./include/Visus/Point.h
	./include/Visus/Polygon.h
	./include/Visus/Position.h
	./include/Visus/Quaternion.h
	./include/Visus/Ray.h
	./include/Visus/Rectangle.h
	./include/Visus/Sphere.h
	./include/Visus/Segment.h
	./src/Ray.cpp
	./src/Matrix.cpp
	./src/Frustum.cpp
	./src/Point.cpp
	./src/Polygon.cpp
	./src/Position.cpp
	./src/LocalCoordinateSystem.cpp
	./src/Quaternion.cpp
	./include/Visus/ConvexHull.h
	./include/Visus/FindRoots.h
	./src/FindRoots.cpp)

set(NetSources
	./src/CloudStorage.cpp
	./src/AzureCloudStorage.cpp
	./src/AmazonCloudStorage.cpp
	./include/Visus/CloudStorage.h
	./include/Visus/AmazonCloudStorage.h
	./include/Visus/AzureCloudStorage.h
	./include/Visus/NetMessage.h
	./include/Visus/NetServer.h
	./include/Visus/NetService.h
	./include/Visus/NetSocket.h
	./src/NetMessage.cpp
	./src/NetServer.cpp
	./src/NetService.cpp
	./src/NetSocket.cpp)

set(ArraySources 
	./include/Visus/DType.h
	./include/Visus/Array.h
	./include/Visus/KdArray.h
	./include/Visus/Field.h
	./include/Visus/Encoders.h
	./include/Visus/Range.h
	./include/Visus/Statistics.h
	./include/Visus/Histogram.h
	./include/Visus/TransferFunction.h
	./src/DType.cpp
	./src/Array.cpp
	./src/KdArray.cpp
	./src/ArrayUtils.cpp
	./src/Encoders.cpp
	./src/Field.cpp
	./src/ArrayPluginDevnull.cpp
	./src/ArrayPluginFreeimage.cpp
	./src/ArrayPluginRawArray.cpp
	./src/Statistics.cpp
	./src/Histogram.cpp
	./src/TransferFunction.cpp
	./src/TransferFunction.Defaults.cpp	
	)

set(MiscSources 
	./include/Visus/PointCloud.h
	./include/Visus/Graph.h
	./include/Visus/UnionFind.h
	./src/PointCloud.cpp)

set(PythonSources 
	./include/Visus/swigpyrun.h 
	./include/Visus/PythonEngine.h
	./src/PythonEngine.cpp)

source_group(""                FILES ${UpLevelSources})
source_group(Core              FILES ${CoreSources})
source_group(Thread            FILES ${ThreadSources})
source_group(Geometry          FILES ${GeometrySources})
source_group(Net               FILES ${NetSources})
source_group(Array             FILES ${ArraySources})
source_group(TransferFunction  FILES ${TransferFunctionSources})
source_group(Misc              FILES ${MiscSources})
source_group(Python            FILES ${PythonSources})

IF (WIN32 OR APPLE)
	file(GLOB Sources include/Visus/*.h src/*.cpp src/*.mm)
else()
	file(GLOB Sources include/Visus/*.h src/*.cpp)
endif()


AddLibrary(VisusKernel ${Sources})

target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../InternalLibs/dtl/include)

if (VISUS_INTERNAL_ZLIB)
	target_link_libraries(VisusKernel PRIVATE zlib)
else()
	target_include_directories(VisusKernel PRIVATE ${ZLIB_INCLUDE_DIRS})
	target_link_libraries(VisusKernel      PUBLIC ${ZLIB_LIBRARIES})	
endif()

if (VISUS_INTERNAL_LZ4)
	target_link_libraries(VisusKernel PRIVATE lz4)
else()
	target_include_directories(VisusKernel PRIVATE ${LZ4_INCLUDE_DIR})
	target_link_libraries(VisusKernel      PUBLIC ${LZ4_LIBRARY})
endif()

if (VISUS_INTERNAL_TINYXML)
	target_link_libraries(VisusKernel PRIVATE TinyXML)
else()
	target_include_directories(VisusKernel PRIVATE ${TinyXML_INCLUDE_DIRS})
	target_link_libraries(VisusKernel      PUBLIC ${TinyXML_LIBRARIES})	
endif()

if (VISUS_INTERNAL_FREEIMAGE)
	target_link_libraries(VisusKernel PRIVATE FreeImage)
else()
	target_include_directories(VisusKernel PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
	target_link_libraries(VisusKernel      PUBLIC ${FREEIMAGE_LIBRARIES})
endif()

if (VISUS_INTERNAL_OPENSSL)
	target_link_libraries(VisusKernel PRIVATE crypto)
	target_link_libraries(VisusKernel PRIVATE ssl)	
else()
	target_include_directories(VisusKernel PRIVATE ${OPENSSL_INCLUDE_DIR})
	target_link_libraries(VisusKernel      PUBLIC ${OPENSSL_SSL_LIBRARY})
	target_link_libraries(VisusKernel      PUBLIC ${OPENSSL_CRYPTO_LIBRARY})
endif()

if (VISUS_INTERNAL_CURL)
	target_link_libraries(VisusKernel PRIVATE curl)
else()
	target_include_directories(VisusKernel PRIVATE ${CURL_INCLUDE_DIR})
	target_link_libraries(VisusKernel      PUBLIC ${CURL_LIBRARIES})
endif()

if (NOT VISUS_INTERNAL_PYTHON)
	target_include_directories(VisusKernel PUBLIC  ${PYTHON_INCLUDE_DIRS})
endif()


if(WIN32)
	target_link_libraries(VisusKernel PUBLIC Psapi.lib Iphlpapi.lib DbgHelp.lib Ws2_32.lib)

elseif(APPLE)
	target_link_libraries(VisusKernel PUBLIC 
		"-framework CoreFoundation" "-framework Foundation" "-framework AppKit" "-framework IOKit"
		"-framework CoreAudio" "-framework CoreMIDI" "-framework QuartzCore" "-framework AudioToolbox")

else()
	target_link_libraries(VisusKernel PUBLIC dl rt util)
        if (HAS_UUID_H)
          target_link_libraries(VisusKernel PUBLIC uuid)
        endif()
	target_link_libraries(VisusKernel PUBLIC  pthread)

endif()

add_custom_command(TARGET VisusKernel POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../CMake/__init__.py  $<TARGET_FILE_DIR:VisusKernel>)
add_custom_command(TARGET VisusKernel POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../..//CMake/OpenVisus.py $<TARGET_FILE_DIR:VisusKernel>)


# link python
if (VISUS_INTERNAL_PYTHON)

	target_link_libraries(VisusKernel PUBLIC python)
	
elseif (WIN32)

	list(FIND PYTHON_LIBRARY optimized __index__)
	if (NOT ${__index__} EQUAL -1)
		math(EXPR __next_index__ "${__index__}+1")
		list(GET PYTHON_LIBRARY ${__next_index__} PYTHON_RELEASE_LIBRARY)
		target_link_libraries(VisusKernel PUBLIC debug     ${PYTHON_DEBUG_LIBRARY})
		target_link_libraries(VisusKernel PUBLIC optimized ${PYTHON_RELEASE_LIBRARY})
	else()
		target_link_libraries(VisusKernel PUBLIC ${PYTHON_LIBRARY})
	endif()
		
else()
	
	# for apple and linux I'm linking python only for Executables (of final shared dll such as  mod_visus) 
	# otherwise I'm going to have multiple libpython in the same process
	# with the error message: PyThreadState_Get: no current thread		
	
	if (APPLE)
		set_target_properties(VisusKernel PROPERTIES LINK_FLAGS "-undefined dynamic_lookup") 	
	endif()				
	
	target_link_libraries(VisusKernel INTERFACE ${PYTHON_LIBRARY})
	
endif()
