find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

# command to generate swigpyrun.h
# swig -python -external-runtime Libs/Kernel/include/swigpyrun.h

set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})

set(CMAKE_SWIG_FLAGS "")

set(SWIG_FLAGS "")
set(SWIG_FLAGS "${SWIG_FLAGS};-threads")
set(SWIG_FLAGS "${SWIG_FLAGS};-extranative")
set(SWIG_FLAGS "${SWIG_FLAGS};-DNUMPY_FOUND=1")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/..")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include")
set(SWIG_FLAGS "${SWIG_FLAGS};-I${CMAKE_CURRENT_SOURCE_DIR}/../AppKit/include")

AddVisusSwigLibrary(VisusKernel      ${CMAKE_CURRENT_SOURCE_DIR}/VisusKernelPy.i)
AddVisusSwigLibrary(VisusDataflow    ${CMAKE_CURRENT_SOURCE_DIR}/VisusDataflowPy.i)
AddVisusSwigLibrary(VisusDb          ${CMAKE_CURRENT_SOURCE_DIR}/VisusDbPy.i)
AddVisusSwigLibrary(VisusIdx         ${CMAKE_CURRENT_SOURCE_DIR}/VisusIdxPy.i)
AddVisusSwigLibrary(VisusNodes       ${CMAKE_CURRENT_SOURCE_DIR}/VisusNodesPy.i)

if (VISUS_GUI)
  AddVisusSwigLibrary(VisusAppKit    ${CMAKE_CURRENT_SOURCE_DIR}/VisusAppKitPy.i)
  AddVisusSwigLibrary(VisusGui       ${CMAKE_CURRENT_SOURCE_DIR}/VisusGuiPy.i)
  AddVisusSwigLibrary(VisusGuiNodes  ${CMAKE_CURRENT_SOURCE_DIR}/VisusGuiNodesPy.i)
endif()
