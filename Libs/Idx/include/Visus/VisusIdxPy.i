%module(directors="1") VisusIdxPy

%{ 
#include <Visus/Idx.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>

//VISUS_DISOWN

//VISUS_NEWOBJECT

%shared_ptr(Visus::IdxDataset)
%shared_ptr(Visus::IdxMultipleDataset)

%import <Visus/VisusKernelPy.i>
%import <Visus/VisusDbPy.i>

%include <Visus/Idx.h>
%include <Visus/IdxFile.h>
%include <Visus/IdxDataset.h>
%include <Visus/IdxMultipleDataset.h>



