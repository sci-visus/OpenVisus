find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

# command to generate swigpyrun.h
# swig -python -external-runtime Libs/Kernel/include/swigpyrun.h

AddVisusSwigLibrary(VisusKernel)
AddVisusSwigLibrary(VisusDataflow)
AddVisusSwigLibrary(VisusDb)
AddVisusSwigLibrary(VisusIdx)
AddVisusSwigLibrary(VisusNodes)

if (VISUS_GUI)
  AddVisusSwigLibrary(VisusAppKit)
  AddVisusSwigLibrary(VisusGui)
  AddVisusSwigLibrary(VisusGuiNodes)
endif()
