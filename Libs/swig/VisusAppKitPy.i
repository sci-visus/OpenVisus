%module(directors="1") VisusAppKitPy

%include <VisusPy.i>

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

%import  <VisusKernelPy.i>

%import  <VisusDataflowPy.i>
%import  <VisusDbPy.i>
%import  <VisusNodesPy.i>
%import  <VisusIdxPy.i>
%import  <VisusGuiPy.i>
%import  <VisusGuiNodesPy.i>

%include <Visus/AppKit.h>
%include <Visus/Viewer.h>




