

find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	add_library(OpenVisus::Kernel   SHARED IMPORTED GLOBAL)
	add_library(OpenVisus::XIdx     SHARED IMPORTED GLOBAL)
	add_library(OpenVisus::Db       SHARED IMPORTED GLOBAL)
	add_library(OpenVisus::Dataflow SHARED IMPORTED GLOBAL)
	add_library(OpenVisus::Nodes    SHARED IMPORTED GLOBAL)

	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
	add_library(OpenVisus::Gui SHARED IMPORTED GLOBAL)

	if (WIN32)
		set_target_properties(OpenVisus::Kernel   PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/lib/VisusKernel.lib")
		set_target_properties(OpenVisus::XIdx     PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/lib/VisusXIdx.lib")
		set_target_properties(OpenVisus::Db       PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/lib/VisusDb.lib")
		set_target_properties(OpenVisus::Dataflow PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/lib/VisusDataflow.lib")
		set_target_properties(OpenVisus::Nodes    PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/lib/VisusNodes.lib")
		set_target_properties(OpenVisus::Gui      PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/lib/VisusGui.lib")

	elseif (APPLE)
		set_target_properties(OpenVisus::Kernel   PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusKernel.dylib")
		set_target_properties(OpenVisus::XIdx     PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusXIdx.dylib")
		set_target_properties(OpenVisus::Db       PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusDb.dylib")
		set_target_properties(OpenVisus::Dataflow PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusDataflow.dylib")
		set_target_properties(OpenVisus::Nodes    PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusNodes.dylib")
		set_target_properties(OpenVisus::Gui      PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusGui.dylib")

	else()
		set_target_properties(OpenVisus::Kernel   PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusKernel.so")
		set_target_properties(OpenVisus::XIdx     PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusXIdx.so")
		set_target_properties(OpenVisus::Db       PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusDb.so")
		set_target_properties(OpenVisus::Dataflow PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusDataflow.so")
		set_target_properties(OpenVisus::Nodes    PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusNodes.so")
		set_target_properties(OpenVisus::Gui      PROPERTIES IMPORTED_IMPLIB     "${OpenVisus_DIR}/bin/libVisusGui.so")

	endif()

	set_target_properties(OpenVisus::Kernel    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Kernel") 
	set_target_properties(OpenVisus::XIdx      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/XIdx") 
	set_target_properties(OpenVisus::Db        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Db") 
	set_target_properties(OpenVisus::Dataflow  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Dataflow") 
	set_target_properties(OpenVisus::Nodes     PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Nodes") 
	set_target_properties(OpenVisus::Gui       PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Gui") 

	set_target_properties(OpenVisus::XIdx      PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Db        PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Dataflow  PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Nodes     PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Db;OpenVisus::Dataflow") 
	set_target_properties(OpenVisus::Gui       PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL") 

endif()



