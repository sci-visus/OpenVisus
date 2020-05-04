%module(directors="1") VisusNodesPy

%{ 
#include <Visus/LogicSamples.h>
#include <Visus/Nodes.h>
#include <Visus/Query.h>
#include <Visus/BlockQuery.h>
#include <Visus/BoxQuery.h>
#include <Visus/PointQuery.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/Dataflow.h>
#include <Visus/CpuPaletteNode.h>
#include <Visus/FieldNode.h>
#include <Visus/ModelViewNode.h>
#include <Visus/PaletteNode.h>
#include <Visus/StatisticsNode.h>
#include <Visus/TimeNode.h>
#include <Visus/DatasetNode.h>
#include <Visus/QueryNode.h>
#include <Visus/KdQueryNode.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>
%import <Visus/VisusKernelPy.i>
%import <Visus/VisusDataflowPy.i>
%import <Visus/VisusDbPy.i>

%include <Visus/Nodes.h>

%feature("director") Visus::CpuPaletteNode;
%feature("director") Visus::FieldNode;
%feature("director") Visus::ModelViewNode;
%feature("director") Visus::PaletteNode;
%feature("director") Visus::StatisticsNode;
%feature("director") Visus::TimeNode;
%feature("director") Visus::DatasetNode;
%feature("director") Visus::QueryNode;
%feature("director") Visus::KdQueryNode;

%include <Visus/CpuPaletteNode.h>
%include <Visus/FieldNode.h>
%include <Visus/ModelViewNode.h>
%include <Visus/PaletteNode.h>
%include <Visus/StatisticsNode.h>
%include <Visus/TimeNode.h>
%include <Visus/DatasetNode.h>
%include <Visus/QueryNode.h>
%include <Visus/KdQueryNode.h>

//not exposing anything, don't know if we need it

