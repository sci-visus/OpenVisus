
source_group("" FILES
	include/Visus/Kernel.h src/Kernel.cpp src/Kernel.mm
	include/Visus/Python.h)

source_group("Core" FILES
	include/Visus/Aborted.h
	include/Visus/Async.h
	include/Visus/ByteOrder.h src/ByteOrder.cpp	
	include/Visus/Color.h src/Color.cpp
	include/Visus/Diff.h src/Diff.cpp
	include/Visus/DirectoryIterator.h src/DirectoryIterator.cpp	 src/DirectoryIterator.mm	
	include/Visus/File.h src/File.cpp
	include/Visus/HeapMemory.h src/HeapMemory.cpp
	include/Visus/ScopedVector.h
	include/Visus/Model.h include/Visus/Model.cpp
	include/Visus/NumericLimits.h
	include/Visus/Path.h src/Path.cpp
	include/Visus/RamResource.h src/RamResource.cpp
	include/Visus/SignalSlot.h
	include/Visus/StringMap.h src/StringMap.cpp
	include/Visus/StringTree.h src/StringTree.cpp
	include/Visus/StringUtils.h src/StringUtils.cpp
	include/Visus/Time.h src/Time.cpp
	include/Visus/Url.h src/Url.cpp
	include/Visus/Utils.h src/Utils.cpp
	include/Visus/SharedLibrary.h src/SharedLibrary.cpp
	include/Visus/json.hpp)

source_group("Thread" FILES
	./include/Visus/CriticalSection.h ./src/CriticalSection.cpp	
	./include/Visus/Semaphore.h ./src/Semaphore.cpp
	./include/Visus/Thread.h ./src/Thread.cpp 
	./include/Visus/ThreadPool.h ./src/ThreadPool.cpp )

source_group("Geometry" FILES
	./include/Visus/Matrix.h  ./src/Matrix.cpp
	./include/Visus/Box.h
	./include/Visus/Circle.h
	./include/Visus/Frustum.h ./src/Frustum.cpp
	./include/Visus/Line.h
	./include/Visus/LinearMap.h
	./include/Visus/LocalCoordinateSystem.h
	./include/Visus/Plane.h
	./include/Visus/Point.h ./src/Point.cpp
	./include/Visus/Polygon.h ./src/Polygon.cpp
	./include/Visus/Position.h ./src/Position.cpp
	./include/Visus/Quaternion.h ./src/Quaternion.cpp
	./include/Visus/Ray.h ./src/Ray.cpp
	./include/Visus/Rectangle.h
	./include/Visus/Sphere.h
	./include/Visus/Segment.h
	./src/LocalCoordinateSystem.cpp
	./include/Visus/ConvexHull.h
	./include/Visus/FindRoots.h ./src/FindRoots.cpp)

source_group("Net" FILES
	./include/Visus/CloudStorage.h ./src/CloudStorage.cpp ./src/AmazonCloudStorage.hxx ./src/AzureCloudStorage.hxx ./src/GoogleCloudStorage.hxx
	./include/Visus/NetMessage.h   ./src/NetMessage.cpp
	./include/Visus/NetServer.h    ./src/NetServer.cpp
	./include/Visus/NetService.h   ./src/NetService.cpp
	./include/Visus/NetSocket.h    ./src/NetSocket.cpp)

source_group("Array"  FILES
	./include/Visus/Range.h
	./include/Visus/DType.h            ./src/DType.cpp
	./include/Visus/Array.h            ./src/Array.cpp
	./include/Visus/KdArray.h          ./src/KdArray.cpp
	./include/Visus/ArrayUtils.h       ./src/ArrayUtils.cpp
	./include/Visus/Field.h            ./src/Field.cpp
	./include/Visus/Statistics.h       ./src/Statistics.cpp
	./include/Visus/Histogram.h        ./src/Histogram.cpp
	./include/Visus/RGBAColorMap.h     ./src/RGBAColorMap.cpp
	./include/Visus/TransferFunction.h ./src/TransferFunction.cpp ./src/TransferFunction.Defaults.cpp)
	
source_group("Array\\Plugin" FILES
	./include/Visus/ArrayPlugin.h
	./src/ArrayPlugin.cpp
	./src/ArrayPluginDevnull.hxx
	./src/ArrayPluginFreeimage.hxx
	./src/ArrayPluginRawArray.hxx)

source_group("Encoder" FILES
	./include/Visus/Encoder.h ./src/Encoder.cpp
	./src/EncoderId.hxx
	./src/EncoderZip.hxx
	./src/EncoderLz4.hxx
	./src/EncoderZfp.hxx
	./src/EncoderFreeImage.hxx)

source_group("Misc" FILES 
	./include/Visus/Graph.h
	./include/Visus/UnionFind.h
	./include/Visus/PointCloud.h ./src/PointCloud.cpp
	./include/Visus/Annotation.h ./src/Annotation.cpp)

IF (WIN32 OR APPLE)
	file(GLOB MMFiles src/*.mm)
endif()

if (VISUS_SLAM)
	file(GLOB SlamSources ../slamcpp/*.h ../slamcpp/*.cpp ../slampy/*.py)
	source_group("slam" FILES ${SlamSources})
endif()

file(GLOB cryptlite_Sources src/cryptlite/*.h src/cryptlite/*.c)
source_group("ExternalLibs\\cryptlite" FILES ${cryptlite_Sources})
	
file(GLOB dtl_Sources src/dtl/*.hpp src/dtl/*.c)
source_group("ExternalLibs\\dtl" FILES ${dtl_Sources})

file(GLOB lz4_Sources src/lz4/*.h src/lz4/*.c)
source_group("ExternalLibs\\lz4" FILES ${lz4_Sources})

file(GLOB tinyxml_Sources src/tinyxml/*.h src/tinyxml/*.cpp)
source_group("ExternalLibs\\tinyxml" FILES ${tinyxml_Sources})

file(GLOB zfp_Sources src/zfp/*.h src/zfp/*.cpp src/zfp/*.inl)
source_group("ExternalLibs\\zfp" FILES ${zfp_Sources})
	
file(GLOB zlib_Sources src/zlib/*.h src/zlib/*.c)
source_group("ExternalLibs\\zlib" FILES ${zlib_Sources})	


function(AssignSourceGroup GroupName StartDir)
	foreach(_it_ IN ITEMS ${ARGN})
		file(RELATIVE_PATH _group_ "${StartDir}" "${_it_}")
		get_filename_component(_group_ "${_group_}" PATH)
		string(REPLACE "../"  ""   _group_  "${_group_}")
		string(REPLACE "../"  ""   _group_  "${_group_}")
		string(REPLACE "../"  ""   _group_  "${_group_}")
		string(REPLACE  "/"  "\\"  _group_  "${_group_}")
		source_group("${GroupName}\\${_group_}" FILES "${_it_}")
	endforeach()
endfunction()

file(GLOB SamplesSources 
	${CMAKE_SOURCE_DIR}/README.md
	${CMAKE_SOURCE_DIR}/CMakeLists.txt
	${CMAKE_SOURCE_DIR}/OpenVisusConfig.cmake
	${CMAKE_SOURCE_DIR}/.github/workflows/BuildOpenVisus.yml
	${CMAKE_SOURCE_DIR}/Libs/CMakeLists.txt
	${CMAKE_SOURCE_DIR}/*.ipynb 
	${CMAKE_SOURCE_DIR}/Samples/jupyter/*.ipynb 
	${CMAKE_SOURCE_DIR}/Samples/python/*.py 
	${CMAKE_SOURCE_DIR}/Samples/python/CloudStorage/*.py) 

AssignSourceGroup(CMakeSourceDir ${CMAKE_SOURCE_DIR} ${SamplesSources})

file(GLOB Sources 
	include/Visus/*.*  src/*.hxx src/*.cpp ${MMFiles} 
	${SlamSources} 
	${SamplesSources}
	${cryptlite_Sources} 
	${dtl_Sources} 
	${lz4_Sources} 
	${tinyxml_Sources} 
	${zfp_Sources} 
	${zlib_Sources})
	
add_library(VisusKernel SHARED ${Sources})
set_target_properties(VisusKernel PROPERTIES FOLDER "")

if (WIN32)
	target_compile_options(VisusKernel PUBLIC -DWIN32_LEAN_AND_MEAN)
	target_compile_options(VisusKernel PUBLIC -D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(VisusKernel PUBLIC /bigobj)
elseif (APPLE)
	target_compile_options(VisusKernel PUBLIC -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
else()
	target_compile_options(VisusKernel PUBLIC -D_FILE_OFFSET_BITS=64)
	target_compile_options(VisusKernel PUBLIC -D_LARGEFILE64_SOURCE=1)
	target_compile_options(VisusKernel PUBLIC -Wno-attributes)	
endif()

target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(VisusKernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ExternalLibs)

if (VISUS_NET)
	target_compile_options(VisusKernel PRIVATE -DVISUS_NET=1)
	target_link_libraries(VisusKernel  PRIVATE curl)
endif()

if (VISUS_IMAGE)
	target_compile_options(VisusKernel PRIVATE -DVISUS_IMAGE=1)
	target_link_libraries(VisusKernel  PRIVATE FreeImage)
endif()

target_compile_definitions(VisusKernel  PRIVATE VISUS_BUILDING_VISUSKERNEL=1)
target_include_directories(VisusKernel  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# -DGIT_REVISION
find_package(Git)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
		OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
	if (NOT GIT_REVISION STREQUAL "")
		message(STATUS "${GIT_EXECUTABLE} Current GIT_REVISION ${GIT_REVISION}")
		target_compile_options(VisusKernel PRIVATE -DGIT_REVISION=${GIT_REVISION})
	endif()
endif()

# link libraries
if(WIN32)
	target_link_libraries(VisusKernel PUBLIC Psapi.lib Iphlpapi.lib DbgHelp.lib Ws2_32.lib)
elseif(APPLE)
	target_link_libraries(VisusKernel PUBLIC "-framework CoreFoundation" "-framework Foundation")
	set_target_properties(VisusKernel PROPERTIES XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME "NO") # fixes problem of "_objc_readClassPair" missing
else()
	target_link_libraries(VisusKernel PUBLIC dl rt util pthread)
endif()

# slam if enabled
if (VISUS_SLAM)
	target_include_directories(VisusKernel PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp)
	target_include_directories(VisusKernel PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/ExternalLibs/eigen)
	target_include_directories(VisusKernel PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/ExternalLibs/g2o)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp/ExternalLibs ${CMAKE_BINARY_DIR}/slamcpp/ExternalLibs)
	target_link_libraries(VisusKernel PRIVATE 
		g2o_core  g2o_solver_eigen  g2o_stuff g2o_types_data g2o_types_icp 
		g2o_types_sba g2o_types_sclam2d g2o_types_sim3 g2o_types_slam2d g2o_types_slam2d_addons g2o_types_slam3d g2o_types_slam3d_addons)
endif()
