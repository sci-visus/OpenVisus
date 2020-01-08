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
using namespace Visus;
%}

//VISUS_DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%feature("director") Visus::VisusViewer;

%import  <Visus/VisusKernelPy.i>

%import  <Visus/VisusDataflowPy.i>
%import  <Visus/VisusDbPy.i>
%import  <Visus/VisusNodesPy.i>
%import  <Visus/VisusGuiPy.i>
%import  <Visus/VisusGuiNodesPy.i>

%include <Visus/AppKit.h>
%include <Visus/Viewer.h>




