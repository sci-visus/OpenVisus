%module(directors="1") VisusDbPy

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>

#include <Visus/Db.h>
#include <Visus/Access.h>
#include <Visus/BlockQuery.h>
#include <Visus/Query.h>
#include <Visus/Dataset.h>

using namespace Visus;
%}

%include <Visus/VisusPy.i>

%import <Visus/VisusKernelPy.i>

ENABLE_SHARED_PTR(Access)
ENABLE_SHARED_PTR(BlockQuery)
ENABLE_SHARED_PTR(Query)
ENABLE_SHARED_PTR(Dataset)

%include <Visus/Db.h>
%include <Visus/Access.h>
%include <Visus/LogicBox.h>
%include <Visus/BlockQuery.h>
%include <Visus/Query.h>
%include <Visus/DatasetBitmask.h>
%include <Visus/Dataset.h>
