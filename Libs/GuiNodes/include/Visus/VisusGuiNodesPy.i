%module(directors="1") VisusGuiNodesPy

%{ 
#include <Visus/Gui.h>
#include <Visus/StringTree.h>
#include <Visus/GLObjects.h>
#include <Visus/Dataflow.h>
#include <Visus/GLCameraNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/PythonNode.h>
#include <Visus/GLCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>

using namespace Visus;
%}

%include <Visus/VisusPy.i>
%import  <Visus/VisusKernelPy.i>
%import  <Visus/VisusDataflowPy.i>
%import  <Visus/VisusGuiPy.i>

#define Q_OBJECT
#define signals public
#define slots   

%include <Visus/GuiNodes.h>

%feature("director") Visus::GLCameraNode;
%feature("director") Visus::IsoContourNode;
%feature("director") Visus::IsoContourRenderNode;
%feature("director") Visus::RenderArrayNode;
%feature("director") Visus::OSPRayRenderNode;
%feature("director") Visus::KdRenderArrayNode;
%feature("director") Visus::JTreeRenderNode;
%feature("director") Visus::PythonNode;

%include <Visus/GLCameraNode.h>

%shared_ptr(Visus::IsoContour)

%include <Visus/IsoContourNode.h>
%include <Visus/IsoContourRenderNode.h>

%include <Visus/RenderArrayNode.h>
%include <Visus/KdRenderArrayNode.h>

///////////////////////////////////////////////////////////////
//allow rendering inside main GLCanvas (problem of multi-inheritance Node and GLObject)
%feature("director") Visus::PythonNode; 
%include <Visus/PythonNode.h>
