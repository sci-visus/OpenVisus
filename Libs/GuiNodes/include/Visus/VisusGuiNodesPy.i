%module(directors="1") VisusGuiNodesPy

%{ 
#include <Visus/Gui.h>
#include <Visus/GLObjects.h>
#include <Visus/PythonNode.h>
#include <Visus/Dataflow.h>
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

///////////////////////////////////////////////////////////////
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

%include <Visus/PythonNode.h>
