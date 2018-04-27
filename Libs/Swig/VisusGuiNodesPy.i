%module(directors="1") VisusGuiNodesPy 


%{
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/GLObjects.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GuiNodes.h>
#include <Visus/GLCameraNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
//#include <Visus/IsoContourShader.h>
//#include <Visus/IsoContourTables.h>
#include <Visus/JTreeRenderNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/PythonNode.h>

using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusGuiPy.i>
%import  <VisusDataflowPy.i>

//SharedPtr
ENABLE_SHARED_PTR(IsoContour)

//disown
/*--*/

//new object
/*--*/

%include <Visus/GuiNodes.h>
%include <Visus/GLCameraNode.h>
%include <Visus/IsoContourNode.h>
%include <Visus/IsoContourRenderNode.h>
//%include <Visus/IsoContourShader.h>
//%include <Visus/IsoContourTables.h>
%include <Visus/JTreeRenderNode.h>
%include <Visus/RenderArrayNode.h>
%include <Visus/KdRenderArrayNode.h>


//allow rendering inside main GLCanvas (problem of multi-inheritance Node and GLObject)
%feature("director") Visus::PythonNode; 
%feature("nodirector") Visus::PythonNode::beginUpdate;
%feature("nodirector") Visus::PythonNode::endUpdate;
%feature("nodirector") Visus::PythonNode::getNodeBounds;
%feature("nodirector") Visus::PythonNode::glSetRenderQueue;
%feature("nodirector") Visus::PythonNode::glGetRenderQueue;
%feature("nodirector") Visus::PythonNode::modelChanged;
%feature("nodirector") Visus::PythonNode::enterInDataflow;
%feature("nodirector") Visus::PythonNode::exitFromDataflow;
%feature("nodirector") Visus::PythonNode::addNodeJob;
%feature("nodirector") Visus::PythonNode::abortProcessing;
%feature("nodirector") Visus::PythonNode::joinProcessing;
%feature("nodirector") Visus::PythonNode::messageHasBeenPublished;
%feature("nodirector") Visus::PythonNode::toBool;
%feature("nodirector") Visus::PythonNode::toInt;
%feature("nodirector") Visus::PythonNode::toInt64;
%feature("nodirector") Visus::PythonNode::toDouble;
%feature("nodirector") Visus::PythonNode::toString;
%feature("nodirector") Visus::PythonNode::writeToObjectStream;
%feature("nodirector") Visus::PythonNode::readFromObjectStream;
%feature("nodirector") Visus::PythonNode::writeToSceneObjectStream;
%feature("nodirector") Visus::PythonNode::readFromSceneObjectStream;

%include <Visus/PythonNode.h>


