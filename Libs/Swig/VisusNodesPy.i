%module(directors="1") VisusNodesPy 

%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/ModVisus.h>
#include <Visus/LegacyDataset.h>
#include <Visus/MultiplexAccess.h>
#include <Visus/FilterAccess.h>
#include <Visus/CloudStorageAccess.h>
#include <Visus/DiskAccess.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/Nodes.h>
#include <Visus/CpuPaletteNode.h>
#include <Visus/FieldNode.h>
#include <Visus/FunnelNode.h>
#include <Visus/JTreeNode.h>
#include <Visus/ModelViewNode.h>
#include <Visus/PaletteNode.h>
#include <Visus/ScriptingNode.h>
#include <Visus/StatisticsNode.h>
#include <Visus/TimeNode.h>
#include <Visus/VoxelScoopNode.h>
#include <Visus/DatasetNode.h>
#include <Visus/QueryNode.h>
#include <Visus/KdQueryNode.h>
using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusIdxPy.i>
%import  <VisusDataflowPy.i>
 
//SharedPtr
/*--*/

//disown
/*--*/

//new object
/*--*/


%include <Visus/Nodes.h>
%include <Visus/CpuPaletteNode.h>
%include <Visus/FieldNode.h>
%include <Visus/FunnelNode.h>
%include <Visus/JTreeNode.h>
%include <Visus/ModelViewNode.h>
%include <Visus/PaletteNode.h>
%include <Visus/ScriptingNode.h>
%include <Visus/StatisticsNode.h>
%include <Visus/TimeNode.h>
%include <Visus/VoxelScoopNode.h>
%include <Visus/DatasetNode.h>
%include <Visus/QueryNode.h>
%include <Visus/KdQueryNode.h>
