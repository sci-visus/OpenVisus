

%{ 
#include <Visus/LogicSamples.h>
#include <Visus/Nodes.h>
#include <Visus/Query.h>
#include <Visus/BlockQuery.h>
#include <Visus/BoxQuery.h>
#include <Visus/PointQuery.h>
#include <Visus/Dataflow.h>
#include <Visus/FieldNode.h>
#include <Visus/ModelViewNode.h>
#include <Visus/PaletteNode.h>
#include <Visus/StatisticsNode.h>
#include <Visus/TimeNode.h>
#include <Visus/DatasetNode.h>
#include <Visus/QueryNode.h>
#include <Visus/KdQueryNode.h>
#include <Visus/IdxDiskAccess.h>

#include <Visus/Dataset.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/GoogleMapsDataset.h>

using namespace Visus;
%}

%include <VisusPy.common>
%import <VisusKernelPy.i>
%import <VisusDataflowPy.i>
%import <VisusDbPy.i>

%include <Visus/Nodes.h>

#if 0
%feature("director") Visus::FieldNode;
%feature("director") Visus::ModelViewNode;
%feature("director") Visus::PaletteNode;
%feature("director") Visus::StatisticsNode;
%feature("director") Visus::TimeNode;
%feature("director") Visus::DatasetNode;
%feature("director") Visus::QueryNode;
%feature("director") Visus::KdQueryNode;
#endif

%include <Visus/FieldNode.h>
%include <Visus/ModelViewNode.h>
%include <Visus/PaletteNode.h>
%include <Visus/StatisticsNode.h>
%include <Visus/TimeNode.h>
%include <Visus/DatasetNode.h>
%include <Visus/QueryNode.h>
%include <Visus/KdQueryNode.h>

//not exposing anything, don't know if we need it
