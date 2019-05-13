
find_package(Git)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
		OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
	add_definitions(-DGIT_REVISION=${GIT_REVISION})
	message(STATUS "${GIT_EXECUTABLE} Current GIT_REVISION ${GIT_REVISION}")
endif()

if (NOT DEFINED VISUS_INTERNAL_DEFAULT)
	set(VISUS_INTERNAL_DEFAULT 1)
endif()


if (VISUS_COMPRESSION)

	# *** zlib dependency ***
	SetIfNotDefined(VISUS_INTERNAL_ZLIB ${VISUS_INTERNAL_DEFAULT})
	if (VISUS_INTERNAL_ZLIB)
		MESSAGE(STATUS "Using internal zlib")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../InternalLibs/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
	else()
		# i'm assuming brew here
		if (APPLE)
			SetIfNotDefined(ZLIB_INCLUDE_DIR /usr/local/opt/zlib/include)
			SetIfNotDefined(ZLIB_LIBRARY     /usr/local/opt/zlib/lib/libz.dylib)
		endif()
		find_package(ZLIB REQUIRED)
		MESSAGE(STATUS "Using external zlib")
		message(STATUS "ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS}")
		message(STATUS "ZLIB_LIBRARIES    ${ZLIB_LIBRARIES}")
	endif()

	# *** lz4 dependency ***
	SetIfNotDefined(VISUS_INTERNAL_LZ4 ${VISUS_INTERNAL_DEFAULT})
	if (VISUS_INTERNAL_LZ4)
		MESSAGE(STATUS "Using internal lz4")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../InternalLibs/lz4 ${CMAKE_CURRENT_BINARY_DIR}/lz4)
	else()
		# i'm assuming brew here
		if (APPLE)
			SetIfNotDefined(LZ4_INCLUDE_DIR /usr/local/opt/lz4/include)
			SetIfNotDefined(LZ4_LIBRARY     /usr/local/opt/lz4/lib/liblz4.dylib)
		endif()
		find_package(LZ4 REQUIRED)
		MESSAGE(STATUS "Using external lz4")
		message(STATUS "LZ4_INCLUDE_DIR ${LZ4_INCLUDE_DIR}")
		message(STATUS "LZ4_LIBRARY     ${LZ4_LIBRARY}")
	endif()
endif()

# *** openssl dependency ***
if (VISUS_NET)	

	SetIfNotDefined(VISUS_INTERNAL_OPENSSL ${VISUS_INTERNAL_DEFAULT})
	if (VISUS_INTERNAL_OPENSSL)
		MESSAGE(STATUS "Using internal LibresSSL")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../InternalLibs/libressl ${CMAKE_CURRENT_BINARY_DIR}/libressl)
	else()
		# i'm assuming brew here	
		if (APPLE)
			SetIfNotDefined(OPENSSL_INCLUDE_DIR    /usr/local/opt/openssl/include)
			SetIfNotDefined(OPENSSL_CRYPTO_LIBRARY /usr/local/opt/openssl/lib/libcrypto.dylib)
			SetIfNotDefined(OPENSSL_SSL_LIBRARY    /usr/local/opt/openssl/lib/libssl.dylib)	
		endif()
		find_package(OpenSSL REQUIRED)
		MESSAGE(STATUS "Using external OpenSSL")
		message(STATUS "OPENSSL_INCLUDE_DIR    ${OPENSSL_INCLUDE_DIR}")
		message(STATUS "OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY}")
		message(STATUS "OPENSSL_SSL_LIBRARY    ${OPENSSL_SSL_LIBRARY}")
	endif()
endif()

# *** curl dependency ***
if (VISUS_NET)		
	SetIfNotDefined(VISUS_INTERNAL_CURL ${VISUS_INTERNAL_DEFAULT})
	if (VISUS_INTERNAL_CURL)
		MESSAGE(STATUS "Using internal Curl")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../InternalLibs/curl ${CMAKE_CURRENT_BINARY_DIR}/curl)
	else()
		# i'm assuming brew here
		if (APPLE)
			SetIfNotDefined(CURL_INCLUDE_DIR /usr/local/opt/curl/include)
			SetIfNotDefined(CURL_LIBRARY     /usr/local/opt/curl/lib/libcurl.dylib)
		endif()
		find_package(CURL REQUIRED)
		# see https://github.com/Microsoft/vcpkg/issues/1909
	 	if (CMAKE_TOOLCHAIN_FILE)
			list(LENGTH CURL_LIBRARY CURL_LIBRARY_LENGTH)
			if (CURL_LIBRARY_LENGTH EQUAL 1)
				set(CURL_LIBRARY_DEBUG_LIB   ${CURL_LIBRARY})
				set(CURL_LIBRARY_RELEASE_LIB ${CURL_LIBRARY_DEBUG_LIB}/../../../lib/libcurl.lib)
				get_filename_component(CURL_LIBRARY_RELEASE_LIB ${CURL_LIBRARY_RELEASE_LIB} REALPATH)
				ForceUnset(CURL_LIBRARY)
				ForceUnset(CURL_LIBRARIES)
				set(CURL_LIBRARY "debug;${CURL_LIBRARY_DEBUG_LIB};optimized;${CURL_LIBRARY_RELEASE_LIB}")
				set(CURL_LIBRARIES ${CURL_LIBRARY})
			endif()
		endif()	
		message(STATUS "Using external Curl")
		message(STATUS "CURL_INCLUDE_DIR ${CURL_INCLUDE_DIR}")
		message(STATUS "CURL_LIBRARIES   ${CURL_LIBRARIES}")
	endif()

endif() 


# *** FreeImage dependency ***
if (VISUS_IMAGE)
	SetIfNotDefined(VISUS_INTERNAL_FREEIMAGE ${VISUS_INTERNAL_DEFAULT})
	if (VISUS_INTERNAL_FREEIMAGE)
		MESSAGE(STATUS "Using internal FreeImage")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../InternalLibs/FreeImage ${CMAKE_CURRENT_BINARY_DIR}/FreeImage)
	else()
		# i'm assuming brew here
		if (APPLE)
			SetIfNotDefined(FREEIMAGE_INCLUDE_DIRS /usr/local/opt/freeimage/include)
			SetIfNotDefined(FREEIMAGE_LIBRARIES    /usr/local/opt/freeimage/lib/libfreeimage.dylib)
		endif()
		find_package(FreeImage REQUIRED)
		MESSAGE(STATUS "Using external FreeImage")
		message(STATUS "FREEIMAGE_INCLUDE_DIRS ${FREEIMAGE_INCLUDE_DIRS}")
		message(STATUS "FREEIMAGE_LIBRARIES    ${FREEIMAGE_LIBRARIES}")
	endif()
	
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


file(GLOB PyFiles ${CMAKE_CURRENT_SOURCE_DIR}/../../CMake/*.py)

set(RootFiles 
	${CMAKE_CURRENT_SOURCE_DIR}/../../README.md
	${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt
	${CMAKE_CURRENT_SOURCE_DIR}/../../build.bat
	${CMAKE_CURRENT_SOURCE_DIR}/../../build.sh
	${CMAKE_CURRENT_SOURCE_DIR}/../../build_docker.sh
	${CMAKE_CURRENT_SOURCE_DIR}/../../build_conda.sh
	${CMAKE_CURRENT_SOURCE_DIR}/../../build_osx.sh
	${CMAKE_CURRENT_SOURCE_DIR}/../../CMake/VisusMacros.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/../../CMake/OpenVisusConfig.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt
)


source_group("PyFiles" FILES ${PyFiles})

source_group("" FILES
	${RootFiles}
	./include/Visus/Visus.h 
	./include/Visus/Kernel.h 
	./src/Visus.cpp 
	./src/Kernel.cpp
	./src/Kernel.mm
)

source_group("Core" FILES
	./include/Visus/Aborted.h
	./include/Visus/Async.h
	./include/Visus/BigInt.h
	./include/Visus/ByteOrder.h
	./src/ByteOrder.cpp	
	./include/Visus/Color.h
	./src/Color.cpp
	./include/Visus/Diff.h
	./src/Diff.cpp
	./include/Visus/DirectoryIterator.h
	./src/DirectoryIterator.cpp	
	./src/DirectoryIterator.mm	
	./include/Visus/File.h
	./src/File.cpp
	./include/Visus/HeapMemory.h
	./include/Visus/UUID.h
	./src/HeapMemory.cpp
	./include/Visus/ScopedVector.h
	./include/Visus/Log.h
	./src/Log.cpp
	./include/Visus/Model.h
	./include/Visus/NumericLimits.h
	./include/Visus/ObjectStream.h
	./src/ObjectStream.cpp
	./include/Visus/Path.h
	./src/Path.cpp
	./include/Visus/RamResource.h
	./src/RamResource.cpp
	./include/Visus/Singleton.h
	./include/Visus/SignalSlot.h
	./include/Visus/StringMap.h
	./src/StringMap.cpp
	./include/Visus/StringTree.h
	./src/StringTree.cpp
	./include/Visus/StringUtils.h
	./src/StringUtils.cpp
	./include/Visus/Time.h
	./src/Time.cpp
	./include/Visus/Url.h
	./src/Url.cpp
	./include/Visus/Utils.h
	./src/Utils.cpp
	./src/UUID.cpp
	./include/Visus/ApplicationStats.h
	./src/ApplicationStats.cpp
	./include/Visus/ApplicationInfo.h
	./src/ApplicationInfo.cpp
	./include/Visus/VisusConfig.h
	./src/VisusConfig.cpp)
	

source_group("Thread" FILES
	./include/Visus/CriticalSection.h
	./src/CriticalSection.cpp	
	./include/Visus/Semaphore.h
	./src/Semaphore.cpp
	./include/Visus/Thread.h
	./src/Thread.cpp 
	./include/Visus/ThreadPool.h
	./src/ThreadPool.cpp 
)

source_group("Geometry" FILES
	./include/Visus/Matrix.h 
	./src/Matrix.cpp
	./include/Visus/Box.h
	./include/Visus/Circle.h
	./include/Visus/Frustum.h
	./src/Frustum.cpp
	./include/Visus/Line.h
	./include/Visus/LinearMap.h
	./include/Visus/LocalCoordinateSystem.h
	./include/Visus/Plane.h
	./include/Visus/Point.h
	./src/Point.cpp
	./include/Visus/Polygon.h
	./src/Polygon.cpp
	./include/Visus/Position.h
	./src/Position.cpp
	./include/Visus/Quaternion.h
	./src/Quaternion.cpp
	./include/Visus/Ray.h
	./src/Ray.cpp
	./include/Visus/Rectangle.h
	./include/Visus/Sphere.h
	./include/Visus/Segment.h
	./src/LocalCoordinateSystem.cpp
	./include/Visus/ConvexHull.h
	./include/Visus/FindRoots.h
	./src/FindRoots.cpp)

source_group("Net" FILES
	./src/AmazonCloudStorage.h
	./src/AzureCloudStorage.h
	./src/GoogleCloudStorage.h
	./include/Visus/CloudStorage.h
	./src/CloudStorage.cpp
	./include/Visus/NetMessage.h
	./src/NetMessage.cpp
	./include/Visus/NetServer.h
	./src/NetServer.cpp
	./include/Visus/NetService.h
	./src/NetService.cpp
	./include/Visus/NetSocket.h
	./src/NetSocket.cpp)

source_group("Array"  FILES
	./include/Visus/Range.h
	./include/Visus/DType.h
	./src/DType.cpp
	./include/Visus/Array.h
	./src/Array.cpp
	./include/Visus/KdArray.h
	./src/KdArray.cpp
	./include/Visus/ArrayUtils.h
	./src/ArrayUtils.cpp
	./include/Visus/Field.h
	./src/Field.cpp
	./include/Visus/Statistics.h
	./src/Statistics.cpp
	./include/Visus/Histogram.h
	./src/Histogram.cpp
	./include/Visus/TransferFunction.h
	./src/TransferFunction.cpp
	./src/TransferFunction.Defaults.cpp)
	
source_group("Array\\Plugin" FILES
	./include/Visus/ArrayPlugin.h
	./src/ArrayPlugin.cpp
	./src/ArrayPluginDevnull.h
	./src/ArrayPluginFreeimage.h
	./src/ArrayPluginRawArray.h)

source_group("Encoder" FILES
	./include/Visus/Encoder.h
	./src/EncoderId.h
	./src/EncoderZip.h
	./src/EncoderLz4.h
	./src/EncoderFreeImage.h
	./src/Encoder.cpp)

source_group("Python" FILES 
	./include/Visus/swigpyrun.h 
	./include/Visus/PythonEngine.h
	./src/PythonEngine.cpp)

source_group("Misc" FILES 
	./include/Visus/PointCloud.h
	./include/Visus/Graph.h
	./include/Visus/UnionFind.h
	./src/PointCloud.cpp)

IF (WIN32 OR APPLE)
	file(GLOB MM_FILES src/*.mm)
endif()

file(GLOB DtlSources   src/dtl/*.hpp src/dtl/*.cpp)
source_group("dtl" FILES ${DtlSources})

file(GLOB TinyXmlSources   src/tinyxml/*.h src/tinyxml/*.cpp)
source_group("tinyxml" FILES ${TinyXmlSources})


file(GLOB Sources 
	${RootFiles}
	${PyFiles}
	include/Visus/*.h 
	include/Visus/*.i 
	src/*.cpp 
	src/*.h
	${TinyXmlSources}
	${DtlSources}
	${MM_FILES} 
)

AddLibrary(VisusKernel ${Sources})

target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# openmp
if (VISUS_OPENMP)

	find_package(OpenMP QUIET)

	# note: to have OpenMP cmake support you need cmake version>=3.9
	if (TARGET OpenMP::OpenMP_CXX)
		MESSAGE(STATUS "OpenMP found (OpenMP::OpenMP_CXX)")
		target_link_libraries(VisusKernel PUBLIC OpenMP::OpenMP_CXX)

	# FindOpenMP.cmake seems broken, at least for me
	# see https://iscinumpy.gitlab.io/post/omp-on-high-sierra/
	elseif (APPLE AND EXISTS "/usr/local/opt/libomp/include/omp.h" AND EXISTS "/usr/local/opt/libomp/lib/libomp.a")
		MESSAGE(STATUS "OpenMP found (brew version for APPLE)")
		target_include_directories(VisusKernel   PUBLIC /usr/local/opt/libomp/include)
		target_link_libraries(VisusKernel        PUBLIC /usr/local/opt/libomp/lib/libomp.a)
		target_compile_options(VisusKernel       PUBLIC -Xpreprocessor -fopenmp)

	endif()

endif()

if (VISUS_COMPRESSION)

	target_compile_definitions(VisusKernel PUBLIC -DVISUS_COMPRESSION=1)

	if (TARGET zlib)
		target_link_libraries(VisusKernel PRIVATE zlib)
	else()
		target_include_directories(VisusKernel PRIVATE ${ZLIB_INCLUDE_DIRS})
		target_link_libraries(VisusKernel      PUBLIC ${ZLIB_LIBRARIES})	
	endif()

	if (TARGET lz4)
		target_link_libraries(VisusKernel PRIVATE lz4)
	else()
		target_include_directories(VisusKernel PRIVATE ${LZ4_INCLUDE_DIR})
		target_link_libraries(VisusKernel      PUBLIC ${LZ4_LIBRARY})
	endif()
endif()

if (VISUS_IMAGE)

	target_compile_definitions(VisusKernel PUBLIC -DVISUS_IMAGE=1)

	if (TARGET FreeImage)
		target_link_libraries(VisusKernel PRIVATE FreeImage)
	else()
		target_include_directories(VisusKernel PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
		target_link_libraries(VisusKernel      PUBLIC ${FREEIMAGE_LIBRARIES})
	endif()
endif()

if (VISUS_NET)

	target_compile_definitions(VisusKernel PUBLIC -DVISUS_NET=1)

	if (TARGET crypto)
	
		target_link_libraries(VisusKernel PRIVATE crypto)
		
		if (TARGET ssl)
			target_link_libraries(VisusKernel PRIVATE ssl)	
		endif()
		
		if (TARGET tls)
			target_link_libraries(VisusKernel PRIVATE tls)
		endif()
		
	else()
		target_include_directories(VisusKernel PRIVATE ${OPENSSL_INCLUDE_DIR})
		target_link_libraries(VisusKernel      PUBLIC ${OPENSSL_SSL_LIBRARY})
		target_link_libraries(VisusKernel      PUBLIC ${OPENSSL_CRYPTO_LIBRARY})
	endif()

	if (TARGET curl)
		target_link_libraries(VisusKernel PRIVATE curl)
	else()
		target_include_directories(VisusKernel PRIVATE ${CURL_INCLUDE_DIR})
		target_link_libraries(VisusKernel      PUBLIC ${CURL_LIBRARIES})
	endif()
endif()

if (VISUS_PYTHON)
	target_compile_definitions(VisusKernel  PUBLIC  -DVISUS_PYTHON=1)
	target_compile_definitions(VisusKernel  PRIVATE SWIG_TYPE_TABLE=OpenVisus)
else()
	target_compile_definitions(VisusKernel PUBLIC -DVISUS_PYTHON=0)
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

AddSwigLibrary(VisusKernel ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusKernelPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")

	  
