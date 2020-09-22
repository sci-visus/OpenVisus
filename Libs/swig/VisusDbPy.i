%module(directors="1") VisusDbPy

%{ 
#include <Visus/Db.h>
#include <Visus/StringTree.h>
#include <Visus/Access.h>
#include <Visus/Query.h>
#include <Visus/BlockQuery.h>
#include <Visus/BoxQuery.h>
#include <Visus/PointQuery.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/Dataset.h>
#include <Visus/ModVisus.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/VisusConvert.h>
#include <PyMultipleDataset.h>
using namespace Visus;
%}

%include <VisusPy.common>

%import <VisusKernelPy.i>

%shared_ptr(Visus::Access)
%shared_ptr(Visus::IdxDiskAccess)
%shared_ptr(Visus::IdxMultipleAccess)

%shared_ptr(Visus::Query)
%shared_ptr(Visus::BlockQuery)
%shared_ptr(Visus::BoxQuery)
%shared_ptr(Visus::PointQuery)

%shared_ptr(Visus::Dataset)
%shared_ptr(Visus::GoogleMapsDataset)
%shared_ptr(Visus::IdxDataset)
%shared_ptr(Visus::IdxMultipleDataset)

%rename(LoadDatasetCpp) Visus::LoadDataset;
%pythoncode %{
def LoadDataset(url):
	db=LoadDatasetCpp(url)
	if not db: return None
	from OpenVisus.dataset import PyDataset
	midx=IdxMultipleDataset.castFrom(db)
	if midx: return PyDataset(midx)
	idx=IdxDataset.castFrom(db)
	if idx: return PyDataset(idx)
	google=GoogleMapsDataset.castFrom(db)
	if google: return PyDataset(google)
	return PyDataset(db)

def LoadIdxDataset(url):
	return LoadDataset(url)
%}

%extend Visus::DbModule {
	static void attach() {
		//user defined attach
		DbModule::attach();
		DatasetFactory::getSingleton()->registerDatasetType("IdxMultipleDataset", []() {return std::make_shared<PyMultipleDataset>(); });
	}
}
%ignore Visus::DbModule::attach;

%include <Visus/Db.h>
%include <Visus/Access.h>
%include <Visus/LogicSamples.h>
%include <Visus/Query.h>
%include <Visus/BlockQuery.h>
%include <Visus/BoxQuery.h>
%include <Visus/PointQuery.h>
%include <Visus/Query.h>
%include <Visus/DatasetBitmask.h>
%include <Visus/DatasetTimesteps.h>
%include <Visus/Dataset.h>
%include <Visus/ModVisus.h>
%include <Visus/Db.h>
%include <Visus/IdxFile.h>
%include <Visus/IdxDataset.h>
%include <Visus/IdxDiskAccess.h>
%include <Visus/IdxMultipleDataset.h>
%include <Visus/VisusConvert.h>




