%module(directors="1") VisusNodesPy

%{ 
#include <Visus/LogicBox.h>
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
#include <Visus/ScriptingNode.h>
#include <Visus/StatisticsNode.h>
#include <Visus/TimeNode.h>
#include <Visus/DatasetNode.h>
#include <Visus/QueryNode.h>
#include <Visus/KdQueryNode.h>
using namespace Visus;
%}

%include <VisusPy.i>
%import <VisusKernelPy.i>
%import <VisusDataflowPy.i>
%import <VisusDbPy.i>
%import <VisusIdxPy.i>

%include <Visus/Nodes.h>
%include <Visus/CpuPaletteNode.h>
%include <Visus/FieldNode.h>
%include <Visus/ModelViewNode.h>
%include <Visus/PaletteNode.h>
%include <Visus/ScriptingNode.h>
%include <Visus/StatisticsNode.h>
%include <Visus/TimeNode.h>
%include <Visus/DatasetNode.h>
%include <Visus/QueryNode.h>
%include <Visus/KdQueryNode.h>

//not exposing anything, don't know if we need it
