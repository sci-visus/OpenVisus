%module(directors="1") VisusIdxPy

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/Idx.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
using namespace Visus;
%}


%include <Visus/VisusPy.i>

%import <Visus/VisusKernelPy.i>
%import <Visus/VisusDbPy.i>

%include <Visus/Idx.h>
%include <Visus/IdxFile.h>
%include <Visus/IdxDataset.h>
%include <Visus/IdxMultipleDataset.h>



