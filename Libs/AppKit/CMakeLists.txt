set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

SET(RootSources
	./PyScriptingNode.py
	./PyViewer.py
	./include/Visus/AppKit.h
	./include/Visus/FreeTransform.h
	./include/Visus/Viewer.h
	./src/AppKit.cpp
	./src/FreeTransform.cpp
	./src/Viewer.cpp
	./src/Viewer.OpenGL.cpp
	./src/Viewer.Net.cpp
	./src/Viewer.Gui.cpp
	./resources/AppKit.qrc
)

set(NodeSources
	./include/Visus/GLCameraNode.h
	./include/Visus/GLCameraNodeView.h
	./src/GLCameraNode.cpp        grep -i G
	./src/IsoContourNode.cpp    
	./include/Visus/JTreeRenderNode.h 
	./include/Visus/JTreeRenderNodeView.h   
	./src/JTreeRenderNode.cpp
	./include/Visus/JTreeNodeView.h  
	./include/Visus/OSPRayRenderNode.h     
	./src/OSPRayRenderNode.cpp   
	./include/Visus/RenderArrayNode.h   
	./src/RenderArrayNode.cpp        
	./include/Visus/IsoContourNode.h
	./include/Visus/IsoContourNodeView.h
	./include/Visus/IsoContourRenderNode.h
	./include/Visus/IsoContourRenderNodeView.h
	./resources/IsoContourShader.glsl
	./src/IsoContourRenderNode.cpp 
	./include/Visus/RenderArrayNodeView.h
	./resources/RenderArrayShader.glsl
	./include/Visus/KdRenderArrayNode.h 
	./src/KdRenderArrayNode.cpp   
	./resources/KdRenderArrayShader.glsl
	./include/Visus/PythonNode.h  
	./src/PythonNode.cpp
)


set(ViewerSources
	./PyScriptingNode.py
	./PyViewer.py
	./src/Viewer.cpp
	./src/Viewer.Gui.cpp
	./src/Viewer.Net.cpp
	./src/Viewer.OpenGL.cpp
	./include/Visus/Viewer.h
	./include/Visus/FreeTransform.h
	./src/FreeTransform.cpp

	./include/Visus/ArrayStatisticsView.h
	./include/Visus/HistogramView.h
	./include/Visus/CpuTransferFunctionNodeView.h
	./include/Visus/DataflowFrameView.h
	./src/DataflowFrameView.cpp
	./include/Visus/DataflowTreeView.h
	./src/DataflowTreeView.cpp
	./include/Visus/ScriptingNodeView.h
	./src/ScriptingNodeView.cpp	
	./include/Visus/DatasetNodeView.h
	./include/Visus/FieldNodeView.h
	./include/Visus/FreeTransformView.h
	./include/Visus/PaletteNodeView.h
	./include/Visus/QueryNodeView.h
	./include/Visus/StatisticsNodeView.h
	./include/Visus/TimeNodeView.h
	./include/Visus/TransferFunctionView.h 
	./include/Visus/VoxelScoopNodeView.h
	./include/Visus/ProcessArrayView.h
)

source_group(""        FILES ${RootSources})
source_group("Nodes"   FILES ${NodeSources})
source_group("Viewer"  FILES ${ViewerSources})

file(GLOB Sources *.py include/Visus/*.h include/Visus/*.i src/*.cpp resources/*.glsl resources/*.qrc)
add_library(VisusAppKit SHARED ${Sources})

set_target_properties(VisusAppKit PROPERTIES FOLDER "")

option(VISUS_OSPRAY "Enable ospray" OFF)
if (VISUS_OSPRAY)
	find_package(ospray 2.0 REQUIRED)
	find_package(ospcommon  REQUIRED)
	target_compile_definitions(VisusAppKit PUBLIC -DVISUS_OSPRAY=1)
	target_link_libraries(VisusAppKit PUBLIC ospray::ospray ospcommon::ospcommon)
endif()

target_link_libraries(VisusAppKit PUBLIC VisusNodes VisusGui VisusDataflow)

target_compile_definitions(VisusAppKit  PRIVATE VISUS_BUILDING_VISUSAPPKIT=1)
target_include_directories(VisusAppKit  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

INSTALL(DIRECTORY ./include/Visus         DESTINATION ${OutputDir}/include/AppKit/)
INSTALL(FILES     ./PyScriptingNode.py     DESTINATION ${OutputDir}/.)
INSTALL(FILES     ./PyViewer.py            DESTINATION ${OutputDir}/.)


AddSwigLibrary(VisusAppKitPy VisusAppKit ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusAppKitPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
