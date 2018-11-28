%module(directors="1") VisusNodesPy

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/LogicBox.h>

#include <Visus/Nodes.h>
using namespace Visus;
%}

%include <VisusPy.i>
%import <VisusKernelPy.i>
%import <VisusDataflowPy.i>
%import <VisusDbPy.i>
%import <VisusIdxPy.i>

%include <Visus/Nodes.h>

//not exposing anything, don't know if we need it
