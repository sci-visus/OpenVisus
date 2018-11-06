CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

project(simple_viewer2d)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenVisus REQUIRED)

if (APPLE)
  set(EXECUTABLE_TYPE "MACOSX_BUNDLE")
endif()

file(GLOB Sources *.h *.cpp)
add_executable(simple_viewer2d ${EXECUTABLE_TYPE} ${Sources})
target_link_libraries(simple_viewer2d PUBLIC OpenVisus::AppKit)

  