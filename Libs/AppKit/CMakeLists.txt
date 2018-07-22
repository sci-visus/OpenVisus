set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

SET(RootSources
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

set(ViewSources
	./include/Visus/ArrayStatisticsView.h
	./include/Visus/HistogramView.h
	./include/Visus/CpuTransferFunctionNodeView.h
	./include/Visus/DataflowFrameView.h
	./include/Visus/DataflowTreeView.h
	./include/Visus/DatasetNodeView.h
	./include/Visus/FieldNodeView.h
	./include/Visus/FreeTransformView.h
	./include/Visus/GLCameraNodeView.h
	./include/Visus/IsoContourNodeView.h
	./include/Visus/IsoContourRenderNodeView.h
	./include/Visus/PaletteNodeView.h
	./include/Visus/QueryNodeView.h
	./include/Visus/RenderArrayNodeView.h
	./include/Visus/ScriptingNodeView.h
	./include/Visus/StatisticsNodeView.h
	./include/Visus/TimeNodeView.h
	./include/Visus/TransferFunctionView.h 
	./include/Visus/JTreeNodeView.h
	./include/Visus/JTreeRenderNodeView.h
	./include/Visus/VoxelScoopNodeView.h
	./include/Visus/ProcessArrayView.h
	./src/DataflowFrameView.cpp
	./src/DataflowTreeView.cpp
)

source_group(""     FILES ${RootSources})
source_group("View" FILES ${ViewSources})

file(GLOB Sources include/Visus/*.h src/*.cpp resources/*.glsl resources/*.qrc)

AddLibrary(VisusAppKit ${Sources})
target_link_libraries(VisusAppKit PUBLIC VisusNodes VisusGuiNodes)

