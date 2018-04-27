%module(directors="1") VisusIdxPy 

%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/LegacyDataset.h>
#include <Visus/ModVisus.h>
#include <Visus/Model.h>
#include <Visus/DiskAccess.h>
#include <Visus/FilterAccess.h>
#include <Visus/MultiplexAccess.h>
#include <Visus/CloudStorageAccess.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/Idx.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusDbPy.i>


//disown
/*--*/

//new object
/*--*/

%include <Visus/Idx.h>
%include <Visus/IdxHzOrder.h>
%include <Visus/IdxFile.h>
%include <Visus/IdxDiskAccess.h>
//%include <Visus/IdxDiskAccessV5.h>
//%include <Visus/IdxDiskAccessV6.h>
%ignore Visus::IdxPointQueryHzAddressConversion;
%include <Visus/IdxDataset.h>
%include <Visus/IdxMultipleDataset.h>

