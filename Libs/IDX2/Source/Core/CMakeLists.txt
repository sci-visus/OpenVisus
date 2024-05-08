cmake_minimum_required(VERSION 3.8)
project(idx2Core CXX)

if (VERSION_2)
  add_subdirectory(faster)
endif()

set(IDX2_CORE_HEADER_FILES
  Algorithm.h
  Args.h
  Array.h
  Assert.h
  BitOps.h
  BitStream.h
  CacheTable.h
  CircularQueue.h
  Codecs.h
  Common.h
  Core.h
  DataSet.h
  DataTypes.h
  DebugBreak.h
  Enum.h
  Error.h
  ErrorCodes.h
  Expected.h
  FileSystem.h
  FileUtilities.h
  Format.h
  Function.h
  HashSet.h
  HashTable.h
  idx2Common.h
  idx2Decode.h
  idx2Encode.h
  idx2Lookup.h
  idx2Read.h
  idx2Write.h
  idx2SparseBricks.h
  InputOutput.h
  LinkedList.h
  Logger.h
  Macros.h
  Math.h
  Memory.h
  MemoryMap.h
  Mutex.h
  Random.h
  ScopeGuard.h
  StackTrace.h
  Statistics.h
  String.h
  Test.h
  Timer.h
  Utilities.h
  VarInt.h
  Volume.h
  nd_volume.h
  nd_wavelet.h
  Wavelet.h
  Zfp.h
  v2/idx2Common_v2.h
  v2/idx2Decode_v2.h
  v2/idx2Encode_v2
  v2/idx2Lookup_v2.h
  v2/idx2Read_v2.h
  v2/idx2Write_v2.h
)

set(IDX2_CORE_SOURCE_FILES
  Args.cpp
  Assert.cpp
  DataSet.cpp
  FileSystem.cpp
  Format.cpp
  idx2Common.cpp
  idx2Decode.cpp
  idx2ParallelDecode.cpp
  idx2Encode.cpp
  idx2Lookup.cpp
  idx2Read.cpp
  idx2ParallelRead.cpp
  idx2Write.cpp
  idx2SparseBricks.cpp
  InputOutput.cpp
  Logger.cpp
  Memory.cpp
  MemoryMap.cpp
  StackTrace.cpp
  String.cpp
  Utilities.cpp
  VarInt.cpp
  Volume.cpp
  nd_volume.cpp
  Wavelet.cpp
  nd_wavelet.cpp
  Zfp.cpp
  v2/idx2Common_v2.cpp
  v2/idx2Encode_v2.cpp
  v2/idx2Lookup_v2.cpp
  v2/idx2Read_v2.cpp
  v2/idx2Write_v2.cpp
)

add_library(idx2Core ${IDX2_CORE_HEADER_FILES} ${IDX2_CORE_HEADER_FILES_V2} ${IDX2_CORE_SOURCE_FILES} ${IDX2_CORE_SOURCE_FILES_V2})
set_target_properties(idx2Core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  PUBLIC_HEADER "${IDX2_CORE_HEADER_FILES} ${IDX2_CORE_HEADER_FILES_V2}")
target_compile_features(idx2Core PUBLIC cxx_std_17)
target_compile_definitions(idx2Core PUBLIC -Didx2_Avx2)
target_compile_definitions(idx2Core PUBLIC "$<$<CONFIG:DEBUG>:idx2_Slow>")

if (VISUS_IDX2)
  target_compile_definitions(idx2Core PUBLIC -DVISUS_IDX2)
endif()

find_package(OpenMP)
if (OpenMP_CXX_FOUND)
  target_link_libraries(idx2Core PUBLIC OpenMP::OpenMP_CXX)
endif()

if (MSVC)
  target_compile_definitions(idx2Core PUBLIC -D_CRT_SECURE_NO_WARNINGS)
  target_compile_options(idx2Core PUBLIC /arch:AVX2 /Zc:preprocessor /Zc:__cplusplus /wd5105)
  target_link_options(idx2Core PUBLIC dbghelp.lib)
elseif (UNIX)
  target_compile_options(idx2Core PUBLIC -Wno-format-zero-length -mavx2)
endif()

#export(TARGETS faster idx2Core NAMESPACE idx2CoreConfig:: FILE idx2CoreConfig.cmake)
if (VERSION_2)
target_link_libraries(idx2Core PUBLIC faster)
install(TARGETS idx2Core faster
	EXPORT idx2CoreConfig
	LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include/Core)
else()
install(TARGETS idx2Core
	EXPORT idx2CoreConfig
	LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include/Core)
endif()

install(EXPORT idx2CoreConfig DESTINATION lib/cmake/idx2)
