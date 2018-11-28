%module(directors="1") VisusAppKitPy

%include <VisusPy.i>

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>

#include <Visus/PythonNode.h>
#include <Visus/Viewer.h>
using namespace Visus;
%}

%import  <VisusKernelPy.i>
%import  <VisusDataflowPy.i>
%import  <VisusDbPy.i>
%import  <VisusNodesPy.i>
%import  <VisusIdxPy.i>
%import  <VisusGuiPy.i>
%import  <VisusGuiNodesPy.i>

%include <Visus/AppKit.h>
%include <Visus/Viewer.h>

