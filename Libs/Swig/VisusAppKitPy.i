%module(directors="1") VisusAppKitPy 

%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/JTreeNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/JTreeRenderNode.h>
#include <Visus/FunnelNode.h>
#include <Visus/VoxelScoopNode.h>
#include <Visus/AppKit.h>
#include <Visus/Viewer.h>

#include <Visus/PythonNode.h>

using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusNodesPy.i>
%import  <VisusGuiNodesPy.i>

//SharedPtr
ENABLE_SHARED_PTR(FreeTransform)
ENABLE_SHARED_PTR(ViewerPlugin)

//disown
/*--*/

//new object
/*--*/


%include <Visus/AppKit.h>
%include <Visus/Viewer.h>

//disabled code
#if 0
%include <Visus/HistogramView.h>
%include <Visus/ArrayStatisticsView.h>
%include <Visus/CpuTransferFunctionNodeView.h>
%include <Visus/DataflowFrameView.h>
%include <Visus/DataflowTreeView.h>
%include <Visus/DatasetNodeView.h>
%include <Visus/FieldNodeView.h>
%include <Visus/FreeTransformView.h>
%include <Visus/GLCameraNodeView.h>
%include <Visus/IsoContourNodeView.h>
%include <Visus/IsoContourRenderNodeView.h>
%include <Visus/JTreeNodeView.h>
%include <Visus/JTreeRenderNodeView.h>
%include <Visus/PaletteNodeView.h>
%include <Visus/ProcessArrayView.h>
%include <Visus/QueryNodeView.h>
%include <Visus/RenderArrayNodeView.h>
%include <Visus/ScriptingNodeView.h>
%include <Visus/StatisticsNodeView.h>
%include <Visus/TimeNodeView.h>
%include <Visus/TransferFunctionView.h>
%include <Visus/VoxelScoopNodeView.h>
%include <Visus/FreeTransform.h>
#endif
