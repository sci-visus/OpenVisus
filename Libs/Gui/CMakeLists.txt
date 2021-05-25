set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(RootSources
	./include/Visus/FreeTransform.h ./src/FreeTransform.cpp
	./include/Visus/Viewer.h        ./src/Viewer.cpp ./src/Viewer.OpenGL.cpp ./src/Viewer.Net.cpp ./src/Viewer.Gui.cpp ./include/Visus/Gui.h ./src/Gui.cpp
	./resources/Gui.qrc)

file(GLOB DataflowSources 
	../Dataflow/include/Visus/*.h  
	../Dataflow/src/*.cpp
	./include/Visus/DataflowFrameView.h
	./include/Visus/DataflowTreeView.h
	./src/DataflowFrameView.cpp
	./src/DataflowTreeView.cpp
)
source_group("Dataflow" FILES ${DataflowSources})

file(GLOB OpenGLSources include/Visus/GL*.h src/GL*.cpp resources/GL*)
file(GLOB QtSources include/Visus/QCanvas2d.h include/Visus/QDoubleSlider.h include/Visus/GuiFactory.h)

file(GLOB NodeSources
	../Nodes/include/Visus/*.h 
	../Nodes/src/*.cpp
	
	./include/Visus/DatasetNodeView.h
	./include/Visus/CpuTransferFunctionNodeView.h
	./include/Visus/GLCameraNode.h 
	./include/Visus/GLCameraNodeView.h
	./src/IsoContourNode.cpp    
	./include/Visus/JTreeRenderNode.h 
	./include/Visus/JTreeRenderNodeView.h   
	./src/JTreeRenderNode.cpp
	./include/Visus/JTreeNodeView.h 
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
	./include/Visus/FieldNodeView.h
	./include/Visus/PaletteNodeView.h
	./include/Visus/QueryNodeView.h
	./include/Visus/StatisticsNodeView.h
	./include/Visus/TimeNodeView.h
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
	./include/Visus/TransferFunctionView.h 
	./include/Visus/ProcessArrayView.h
)

source_group(""        FILES ${RootSources})
source_group("Nodes"   FILES ${NodeSources})
source_group("Viewer"  FILES ${ViewerSources})
source_group("Qt"      FILES ${QtSources})
source_group("OpenGL"  FILES ${OpenGLSources})

file(GLOB Sources include/Visus/*.h src/*.cpp resources/*.glsl resources/*.qrc ${DataflowSources} ${NodeSources})

add_library(VisusGui SHARED ${Sources})

target_compile_definitions(VisusGui  PRIVATE VISUS_BUILDING_VISUSGUI=1 VISUS_BUILDING_VISUSDATAFLOW=1 VISUS_BUILDING_VISUSNODES=1)
target_compile_definitions(VisusGui  PRIVATE VISUS_OPENGL_ES=0)

target_include_directories(VisusGui  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(VisusGui  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include>)
target_include_directories(VisusGui  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include>)

if (VISUS_OSPRAY)
	target_compile_definitions(VisusGui PUBLIC -DVISUS_OSPRAY=1)
	target_link_libraries(VisusGui PUBLIC ospray::ospray ospcommon::ospcommon)
endif()

target_link_libraries(VisusGui PUBLIC VisusDb VisusKernel)
target_link_libraries(VisusGui PUBLIC Qt5::Core Qt5::Widgets Qt5::Gui Qt5::OpenGL)

if (WIN32)
	target_link_libraries(VisusGui PUBLIC opengl32.lib glu32.lib)
	
elseif (APPLE)
	target_link_libraries(VisusGui PUBLIC 
		"-framework OpenGL" 
		"-framework AGL"
		"-framework AppKit" 
		"-framework IOKit"
		"-framework CoreAudio" 
		"-framework CoreMIDI" 
		"-framework QuartzCore" 
		"-framework AudioToolbox")
	
else() 
	target_link_libraries(VisusGui PUBLIC GL GLU X11)
	
endif()

set_target_properties(VisusGui PROPERTIES FOLDER "")

