%module(directors="1") VisusAppKitPy

%include <Visus/VisusPy.i>

%{ 
#include <Visus/PythonNode.h>
#include <Visus/Viewer.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/Nodes.h>
using namespace Visus;
%}

//VISUS_DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%import  <Visus/VisusKernelPy.i>

%import  <Visus/VisusDataflowPy.i>
%import  <Visus/VisusDbPy.i>
%import  <Visus/VisusNodesPy.i>
%import  <Visus/VisusIdxPy.i>
%import  <Visus/VisusGuiPy.i>
%import  <Visus/VisusGuiNodesPy.i>

%include <Visus/AppKit.h>
%include <Visus/Viewer.h>




