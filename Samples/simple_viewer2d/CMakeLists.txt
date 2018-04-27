CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

project(simple_viewer2d)

set(CMAKE_CXX_STANDARD 11)

find_package(Visus REQUIRED)

file(GLOB Sources *.h *.cpp)

if (APPLE)
  set(EXECUTABLE_TYPE "MACOSX_BUNDLE")
endif()

add_executable(external_simple_viewer2d ${EXECUTABLE_TYPE} ${Sources})

find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL  REQUIRED)

target_link_libraries(external_simple_viewer2d VisusKernel VisusIdx VisusDb VisusGui VisusGuiNodes VisusAppKit)
 