%module(directors="1") VisusDbPy 

%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/Model.h>
#include <Visus/Db.h>
#include <Visus/Access.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/DiskAccess.h>
#include <Visus/NetworkAccess.h>
#include <Visus/CloudStorageAccess.h>
#include <Visus/FilterAccess.h>
#include <Visus/MultiplexAccess.h>
#include <Visus/RamAccess.h>
#include <Visus/BaseQuery.h>
#include <Visus/BlockQuery.h>
#include <Visus/Query.h>
#include <Visus/DatasetBitmask.h>
#include <Visus/DatasetFilter.h>
#include <Visus/DatasetTimeSteps.h>
#include <Visus/Dataset.h>
#include <Visus/DatasetArrayPlugin.h>
#include <Visus/LegacyDataset.h>
#include <Visus/ModVisus.h>
using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusKernelPy.i>

//SharedPtr
ENABLE_SHARED_PTR(Access)
ENABLE_SHARED_PTR(BlockQuery)
ENABLE_SHARED_PTR(Dataset)
ENABLE_SHARED_PTR(DatasetFilter)
ENABLE_SHARED_PTR(Query)
ENABLE_SHARED_PTR(RamAccess)

//new object
/*--*/

//disown
/*--*/


%include <Visus/Db.h>
%include <Visus/Access.h>
%include <Visus/ModVisusAccess.h>
%include <Visus/DiskAccess.h>
%include <Visus/NetworkAccess.h>
%include <Visus/CloudStorageAccess.h>
%include <Visus/FilterAccess.h>
%include <Visus/MultiplexAccess.h>
%include <Visus/RamAccess.h>
%include <Visus/BaseQuery.h>
%include <Visus/BlockQuery.h>
%include <Visus/Query.h>
%include <Visus/DatasetBitmask.h>
%include <Visus/DatasetFilter.h>
%include <Visus/DatasetTimeSteps.h>
%include <Visus/Dataset.h>
%include <Visus/DatasetArrayPlugin.h>
%include <Visus/LegacyDataset.h>
%include <Visus/ModVisus.h>
