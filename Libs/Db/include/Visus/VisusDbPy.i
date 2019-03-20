%module(directors="1") VisusDbPy

%{ 
#include <Visus/Db.h>
#include <Visus/Access.h>
#include <Visus/BlockQuery.h>
#include <Visus/Query.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/DatasetFilter.h>
#include <Visus/Dataset.h>

using namespace Visus;
%}

%include <Visus/VisusPy.i>

%import <Visus/VisusKernelPy.i>

%shared_ptr(Visus::Access)
%shared_ptr(Visus::BlockQuery)
%shared_ptr(Visus::Query)
%shared_ptr(Visus::BaseDataset)
%shared_ptr(Visus::Dataset)
%shared_ptr(Visus::GoogleMapsDataset)

%include <Visus/Db.h>
%include <Visus/Access.h>
%include <Visus/LogicBox.h>
%include <Visus/BlockQuery.h>
%include <Visus/Query.h>
%include <Visus/DatasetBitmask.h>
%include <Visus/DatasetTimesteps.h>
%include <Visus/DatasetFilter.h>
%include <Visus/Dataset.h>
