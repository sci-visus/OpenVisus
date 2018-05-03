

FindGitRevision()
add_definitions(-DGIT_REVISION=\"${GIT_REVISION}\")

if (DEFINED VISUS_CACHE_PATH)
	add_definitions(-DVISUS_CACHE_PATH="${VISUS_CACHE_PATH}")
endif()

if (DEFINED VISUS_DEFAULT_CONFIG_FILE)
	add_definitions(-DVISUS_DEFAULT_CONFIG_FILE="${VISUS_DEFAULT_CONFIG_FILE}")
endif()

if (DEFINED VISUS_LOG_FILE)
	add_definitions(-DVISUS_LOG_FILE="${VISUS_LOG_FILE}")
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
	./src/Histogram.cpp)

set(MiscSources 
	./include/Visus/TransferFunction.h
	./src/TransferFunction.cpp
	./src/TransferFunction.Defaults.cpp
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

file(GLOB PythonScriptSource ${CMAKE_CURRENT_SOURCE_DIR}/../../Executable/python/*.py)
set(Sources ${Sources} ${PythonScriptSource})
source_group("" FILES ${PythonScriptSource})

file(GLOB RootDocs ../../CMakeLists.txt ../../.*.yml ./../README.md ../../CMake/* ../../ExternalLibs/CMakeLists.txt)
set(Sources ${Sources} ${RootDocs})
source_group("" FILES ${RootDocs})

add_library(VisusKernel ${Sources})

foreach( _it_ ${PythonScriptSource})
    add_custom_command(TARGET VisusKernel POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${_it_} $<TARGET_FILE_DIR:VisusKernel>)
endforeach()

target_compile_definitions(VisusKernel PRIVATE VISUS_BUILDING_VISUSKERNEL=1)

target_include_directories(VisusKernel PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ExternalLibs/dtl/include)

#  zlib
if (TARGET zlib)
	target_link_libraries(VisusKernel PRIVATE  zlib)
else()
	FindZLib()
	target_include_directories(VisusKernel PRIVATE ${ZLIB_INCLUDE_DIRS})
	target_link_libraries(VisusKernel PRIVATE  ${ZLIB_LIBRARIES})	
endif()

# lz4
if (TARGET lz4)
	target_link_libraries(VisusKernel PRIVATE lz4)
else()
	FindLZ4()
	target_include_directories(VisusKernel PRIVATE ${LZ4_INCLUDE_DIR})
	target_link_libraries(VisusKernel PRIVATE  ${LZ4_LIBRARY})		
endif()

#  tinyxml
if (TARGET tinyxml)
	target_link_libraries(VisusKernel PRIVATE  tinyxml)
else()
	FindTinyXml()
	target_include_directories(VisusKernel PRIVATE ${TinyXML_INCLUDE_DIRS})
	target_link_libraries(VisusKernel PRIVATE  ${TinyXML_LIBRARIES})			
endif()

#  FreeImage
if (TARGET FreeImage)
	target_link_libraries(VisusKernel PRIVATE FreeImage)
else()
	FindFreeImage()
	target_include_directories(VisusKernel PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
	target_link_libraries(VisusKernel PRIVATE  ${FREEIMAGE_LIBRARIES})
endif()

# curl
if (TARGET curl)
	target_link_libraries(VisusKernel PRIVATE curl)
else()
	FindCurl()
	target_include_directories(VisusKernel PRIVATE ${CURL_INCLUDE_DIR})
	target_link_libraries(VisusKernel PRIVATE  ${CURL_LIBRARIES})
endif()

#  OpenSSL
if (TARGET ssl)
	target_link_libraries(VisusKernel PRIVATE crypto ssl)
else()
	FindOpenSSL()
	target_include_directories(VisusKernel PRIVATE ${OPENSSL_INCLUDE_DIR})
	target_link_libraries(VisusKernel PRIVATE  ${OPENSSL_SSL_LIBRARY})
	target_link_libraries(VisusKernel PRIVATE  ${OPENSSL_CRYPTO_LIBRARY})
endif()



# python 
if (TARGET python27)
	target_link_libraries(VisusKernel PUBLIC python27)
else()
	target_include_directories(VisusKernel PUBLIC  ${PYTHON_INCLUDE_DIRS})
	target_link_libraries(VisusKernel      PUBLIC  ${PYTHON_LIBRARIES})
	#DisablePythonDebug()
endif()

if(WIN32)
	target_link_libraries(VisusKernel PUBLIC Psapi.lib Iphlpapi.lib DbgHelp.lib Ws2_32.lib)

elseif(APPLE)
	target_link_libraries(VisusKernel PUBLIC 
		"-framework CoreFoundation" "-framework Foundation" "-framework AppKit" "-framework IOKit"
		"-framework CoreAudio" "-framework CoreMIDI" "-framework QuartzCore" "-framework AudioToolbox")

else()
	target_link_libraries(VisusKernel PRIVATE dl rt uuid)
	target_link_libraries(VisusKernel PUBLIC  pthread)

endif()












