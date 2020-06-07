
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/slam.cpp")
	set(VISUS_SLAM 1)
endif()

source_group("" FILES
	./include/Visus/Kernel.h 
	./src/Kernel.cpp
	./src/Kernel.mm
	../../OpenVisusConfig.cmake
	../../.github/workflows/BuildOpenVisus.yml
)

source_group("Core" FILES
	./include/Visus/Aborted.h
	./include/Visus/Async.h
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
	./include/Visus/Model.h
	./include/Visus/Model.cpp
	./include/Visus/NumericLimits.h
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
	./include/Visus/RGBAColorMap.h
	./src/RGBAColorMap.cpp
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
	./src/EncoderZfp.hxx
	./src/EncoderFreeImage.hxx)


source_group("Misc" FILES 
	./include/Visus/PointCloud.h
	./include/Visus/Graph.h
	./include/Visus/UnionFind.h
	./src/PointCloud.cpp
	./include/Visus/Annotation.h
	./src/Annotation.cpp)


if (VISUS_PYTHON)
	file(GLOB PyFiles      
		./include/Visus/swigpyrun.h  
		./include/Visus/Python.h )
	source_group("Python" FILES ${PyFiles})
endif()

file(GLOB RootFiles  
	../../.github/workflows/BuildOpenVisus.yml
	../../CMakeLists.txt
	../../ExternalLibs/CMakeLists.txt
	../../OpenVisusConfig.cmake
	../../LICENCE
	../../README.md*)
source_group("" FILES   ${RootFiles})

file(GLOB_RECURSE ScriptsFiles ../../scripts/*)
AssignSourceGroup(${ScriptsFiles})

file(GLOB_RECURSE  SamplesFiles  ../../Samples/*)
AssignSourceGroup(${SamplesFiles})

IF (WIN32 OR APPLE)
	file(GLOB MMFiles src/*.mm)
endif()

if (VISUS_SLAM)
	file(GLOB SlamCppSources ../slamcpp/*.h ../slamcpp/*.cpp)
	source_group("slamcpp" FILES ${SlamCppSources})
endif()

file(GLOB Sources 
	${RootFiles}
	${PyFiles} 
	${ScriptsFiles}
	${SamplesFiles}
	${MMFiles}
	${SlamCppSources}
	include/Visus/*.*  
	src/*.hxx
	src/*.cpp)

add_library(VisusKernel SHARED ${Sources})
set_target_properties(VisusKernel PROPERTIES FOLDER "")

if (WIN32)
	target_compile_options(VisusKernel PUBLIC /bigobj)		
	target_compile_options(VisusKernel PUBLIC -DPSAPI_VERSION=1)
	target_compile_options(VisusKernel PUBLIC -DFD_SETSIZE=4096)
	target_compile_options(VisusKernel PUBLIC -D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(VisusKernel PUBLIC -DWIN32_LEAN_AND_MEAN)		
elseif (APPLE)
	target_compile_options(VisusKernel PUBLIC -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
	# see https://stackoverflow.com/questions/57343408/cmake-dragndrop-framework-rpath-for-qt-not-set-when-targeting-package

else()
	target_compile_options(VisusKernel PUBLIC -D_FILE_OFFSET_BITS=64)
	target_compile_options(VisusKernel PUBLIC -Wno-attributes)	
endif()

target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ExternalLibs)

target_link_libraries(VisusKernel PRIVATE cryptlite dtl lz4 tinyxml zlib zfp)

if (VISUS_NET)
	target_compile_options(VisusKernel PRIVATE -DVISUS_NET=1)
	target_link_libraries(VisusKernel PRIVATE curl)
endif()

if (VISUS_IMAGE)
	target_compile_options(VisusKernel PRIVATE -DVISUS_IMAGE=1)
	target_link_libraries(VisusKernel PRIVATE FreeImage)
endif()

if (VISUS_PYTHON)
	if (WIN32)
		# I'm always linking to python Release (most of wheel do not work in debug mode, like numpy)
		target_include_directories(VisusKernel PUBLIC ${Python_INCLUDE_DIRS})
		target_compile_definitions(VisusKernel PUBLIC SWIG_PYTHON_INTERPRETER_NO_DEBUG=1)
		target_link_libraries(VisusKernel      PUBLIC ${Python_LIBRARY_RELEASE})
	else()
		target_link_libraries(VisusKernel PUBLIC Python::Module)
	endif()

	target_compile_definitions(VisusKernel  PUBLIC VISUS_PYTHON=1)
	target_compile_definitions(VisusKernel  PUBLIC SWIG_TYPE_TABLE=OpenVisus)
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

target_compile_definitions(VisusKernel  PRIVATE VISUS_BUILDING_VISUSKERNEL=1)
target_include_directories(VisusKernel  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if (VISUS_SLAM)

	target_include_directories(VisusKernel PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp)
	target_include_directories(VisusKernel PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/ExternalLibs/eigen)
	target_include_directories(VisusKernel PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/ExternalLibs/g2o)
	
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/ExternalLibs ${CMAKE_BINARY_DIR}/slamcpp/ExternalLibs)

	target_link_libraries(VisusKernel PRIVATE 	
		g2o_core 
		g2o_solver_eigen 
		g2o_stuff 
		g2o_types_data 
		g2o_types_icp 
		g2o_types_sba 
		g2o_types_sclam2d 
		g2o_types_sim3 
		g2o_types_slam2d 
		g2o_types_slam2d_addons 
		g2o_types_slam3d 
		g2o_types_slam3d_addons)
endif()
