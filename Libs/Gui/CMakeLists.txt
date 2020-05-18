set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(VISUS_OSPRAY "Enable ospray" OFF)
if (VISUS_OSPRAY)
	find_package(ospray 2.0 REQUIRED)
	find_package(ospcommon  REQUIRED)
endif()

SET(RootSources
	./PyScriptingNode.py
	./PyViewer.py
	./include/Visus/FreeTransform.h
	./src/FreeTransform.cpp
	./include/Visus/Viewer.h
	./src/Viewer.cpp
	./src/Viewer.OpenGL.cpp
	./src/Viewer.Net.cpp
	./src/Viewer.Gui.cpp
	./include/Visus/Gui.h
	./src/Gui.cpp
	./resources/Gui.qrc
)


file(GLOB OpenGLSources include/Visus/GL*.h src/GL*.cpp resources/GL*)
file(GLOB QtSources include/Visus/QCanvas2d.h include/Visus/QDoubleSlider.h include/Visus/GuiFactory.h)

set(NodeSources
	./include/Visus/GLCameraNode.h
	./include/Visus/GLCameraNodeView.h
	./src/GLCameraNode.cpp       
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
	./include/Visus/JTreeNode.h  
	./src/JTreeNode.cpp
	./include/Visus/VoxelScoopNode.h  
	./src/VoxelScoopNode.cpp
	./include/Visus/VoxelScoopNodeView.h
	./include/Visus/ScriptingNode.h  
	./src/ScriptingNode.cpp
	./include/Visus/ScriptingNodeView.h
	./src/ScriptingNodeView.cpp
)


set(ViewerSources
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
	./include/Visus/DatasetNodeView.h
	./include/Visus/FieldNodeView.h
	./include/Visus/PaletteNodeView.h
	./include/Visus/QueryNodeView.h
	./include/Visus/StatisticsNodeView.h
	./include/Visus/TimeNodeView.h
	./include/Visus/TransferFunctionView.h 
	./include/Visus/ProcessArrayView.h
)

source_group(""        FILES ${RootSources})
source_group("Nodes"   FILES ${NodeSources})
source_group("Viewer"  FILES ${ViewerSources})
source_group("Qt"      FILES ${QtSources})
source_group("OpenGL"  FILES ${OpenGLSources})


file(GLOB Sources *.py include/Visus/*.h include/Visus/*.i src/*.cpp resources/*.glsl resources/*.qrc)
add_library(VisusGui SHARED ${Sources})
target_compile_definitions(VisusGui  PRIVATE VISUS_BUILDING_VISUSGUI=1)
target_compile_definitions(VisusGui  PRIVATE VISUS_OPENGL_ES=0)
target_include_directories(VisusGui  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

option(VISUS_OSPRAY "Enable ospray" OFF)
if (VISUS_OSPRAY)
	target_compile_definitions(VisusGui PUBLIC -DVISUS_OSPRAY=1)
	target_link_libraries(VisusGui PUBLIC ospray::ospray ospcommon::ospcommon)
endif()

target_link_libraries(VisusGui PUBLIC VisusNodes VisusDb VisusDataflow VisusXIdx VisusKernel)
target_link_libraries(VisusGui PUBLIC Qt5::Core Qt5::Widgets Qt5::Gui Qt5::OpenGL)

if (WIN32)
	target_link_libraries(VisusGui PUBLIC opengl32.lib glu32.lib)
elseif (APPLE)
	target_link_libraries(VisusGui PUBLIC "-framework OpenGL" "-framework AGL")
else() 
	target_link_libraries(VisusGui PUBLIC GL GLU X11)
endif()

set_target_properties(VisusGui PROPERTIES FOLDER "")

CopyToBinaryDirectory(TARGET VisusGui DIRECTORY ./include/Visus    DESTINATION ./include/Gui/Visus)
CopyToBinaryDirectory(TARGET VisusGui FILES PyScriptingNode.py     DESTINATION ./)
CopyToBinaryDirectory(TARGET VisusGui FILES PyViewer.py            DESTINATION ./)

if (VISUS_PYTHON)
	AddSwigLibrary(VisusGuiPy VisusGui ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusGuiPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()