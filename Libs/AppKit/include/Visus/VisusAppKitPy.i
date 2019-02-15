%module(directors="1") VisusAppKitPy

%include <Visus/VisusPy.i>

%{ 
#include <Visus/PythonNode.h>
#include <Visus/Viewer.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
using namespace Visus;
%}

%import  <Visus/VisusKernelPy.i>
%import  <Visus/VisusDataflowPy.i>
%import  <Visus/VisusDbPy.i>
%import  <Visus/VisusNodesPy.i>
%import  <Visus/VisusIdxPy.i>
%import  <Visus/VisusGuiPy.i>
%import  <Visus/VisusGuiNodesPy.i>

%include <Visus/AppKit.h>
%include <Visus/Viewer.h>




