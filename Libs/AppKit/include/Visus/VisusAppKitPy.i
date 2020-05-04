%module(directors="1") VisusAppKitPy

%include <Visus/VisusPy.i>

%{ 
#include <Visus/PythonNode.h>
#include <Visus/Viewer.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/Nodes.h>
#include <Visus/GLCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/StringTree.h>
#include <Visus/Gui.h>
#include <Visus/StringTree.h>
#include <Visus/GLObjects.h>
#include <Visus/Dataflow.h>
#include <Visus/GLCameraNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/ScriptingNode.h>
#include <Visus/PythonNode.h>
#include <Visus/GLCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>
using namespace Visus;
%}

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%feature("director") Visus::Viewer;

%import  <Visus/VisusKernelPy.i>

%import  <Visus/VisusDataflowPy.i>
%import  <Visus/VisusDbPy.i>
%import  <Visus/VisusNodesPy.i>
%import  <Visus/VisusGuiPy.i>


#define Q_OBJECT
#define signals public
#define slots   

%feature("director") Visus::GLCameraNode;
%feature("director") Visus::IsoContourNode;
%feature("director") Visus::IsoContourRenderNode;
%feature("director") Visus::RenderArrayNode;
%feature("director") Visus::OSPRayRenderNode;
%feature("director") Visus::KdRenderArrayNode;
%feature("director") Visus::JTreeRenderNode;
%feature("director") Visus::ScriptingNode;
%feature("director") Visus::PythonNode;

%include <Visus/AppKit.h>

%include <Visus/GLCameraNode.h>

%shared_ptr(Visus::IsoContour)

%include <Visus/IsoContourNode.h>
%include <Visus/IsoContourRenderNode.h>

%include <Visus/RenderArrayNode.h>
%include <Visus/KdRenderArrayNode.h>

///////////////////////////////////////////////////////////////
//allow rendering inside main GLCanvas (problem of multi-inheritance Node and GLObject)
%include <Visus/ScriptingNode.h>
%include <Visus/PythonNode.h>

%include <Visus/Viewer.h>
