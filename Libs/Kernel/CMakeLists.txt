
find_package(Git)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
		OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
		
	if (NOT GIT_REVISION STREQUAL "")
		add_definitions(-DGIT_REVISION=${GIT_REVISION})
		message(STATUS "${GIT_EXECUTABLE} Current GIT_REVISION ${GIT_REVISION}")
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

source_group("" FILES
	./include/Visus/Kernel.h 
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
	./src/VisusConfig.cpp
	./include/Visus/SharedLibrary.h
	./src/SharedLibrary.cpp
	./include/Visus/json.hpp)
	

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
	./include/Visus/CloudStorage.h
	./src/CloudStorage.cpp
	./src/AmazonCloudStorage.hxx
	./src/AzureCloudStorage.hxx
	./src/GoogleCloudStorage.hxx
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
	./src/ArrayPluginDevnull.hxx
	./src/ArrayPluginFreeimage.hxx
	./src/ArrayPluginRawArray.hxx)

source_group("Encoder" FILES
	./include/Visus/Encoder.h
	./src/Encoder.cpp
	./src/EncoderId.hxx
	./src/EncoderZip.hxx
	./src/EncoderLz4.hxx
	./src/EncoderFreeImage.hxx)


source_group("Misc" FILES 
	./include/Visus/PointCloud.h
	./include/Visus/Graph.h
	./include/Visus/UnionFind.h
	./src/PointCloud.cpp
	./include/Visus/Annotation.h
	./src/Annotation.cpp)

IF (WIN32 OR APPLE)
	file(GLOB MM_FILES src/*.mm)
endif()


file(GLOB PyFiles       ./include/Visus/swigpyrun.h  ./include/Visus/Python.h ./src/Python.cpp)
file(GLOB CMakeFiles    ../CMakeList.txt ../../CMakeLists.txt ../../CMake/*)
file(GLOB PyExamples    ../../Samples/python/*.py)

source_group("CMake"           FILES   ${CMakeFiles})
source_group("Python"          FILES   ${PyFiles})
source_group("Python\\Samples" FILES ${PyExamples})
	
file(GLOB Sources 
	${PyFiles} 
	${CMakeFiles} 
	${PyExamples}
	${MM_FILES}
	include/Visus/*.* 
	src/*.cpp src/*.hxx
)

AddLibrary(VisusKernel SHARED ${Sources})

target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ExternalLibs)

target_link_libraries(VisusKernel PRIVATE cryptlite dtl lz4 tinyxml zlib)


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

if (VISUS_IMAGE)
	target_compile_definitions(VisusKernel PUBLIC -DVISUS_IMAGE=1)
	target_link_libraries(VisusKernel PRIVATE FreeImage)
endif()

if (VISUS_NET)
	target_compile_definitions(VisusKernel PUBLIC -DVISUS_NET=1)
	target_link_libraries(VisusKernel PRIVATE curl)
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

if (VISUS_PYTHON)
	AddSwigLibrary(VisusKernel ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusKernelPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()


