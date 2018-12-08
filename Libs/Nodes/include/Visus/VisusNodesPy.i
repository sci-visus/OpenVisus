%module(directors="1") VisusNodesPy

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/LogicBox.h>

#include <Visus/Nodes.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>
%import <Visus/VisusKernelPy.i>
%import <Visus/VisusDataflowPy.i>
%import <Visus/VisusDbPy.i>
%import <Visus/VisusIdxPy.i>

%include <Visus/Nodes.h>

//not exposing anything, don't know if we need it
