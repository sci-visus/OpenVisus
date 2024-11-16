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

%typemap(out) std::shared_ptr<Visus::Dataset>  
{
  //Dataset typemape DSTMP01
  if(auto midx=std::dynamic_pointer_cast<IdxMultipleDataset>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  IdxMultipleDataset >(midx) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__IdxMultipleDataset_t, SWIG_POINTER_OWN);

  else if(auto idx=std::dynamic_pointer_cast<IdxDataset>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  IdxDataset >(idx) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__IdxDataset_t, SWIG_POINTER_OWN);

  else if(auto google=std::dynamic_pointer_cast<GoogleMapsDataset>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  GoogleMapsDataset >(google) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__GoogleMapsDataset_t, SWIG_POINTER_OWN);

  else
	$result= SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  Dataset >(result) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__Dataset_t, SWIG_POINTER_OWN);
}


%pythoncode %{
def LoadDataset(url, cache_dir=""):
	from OpenVisus.dataset import PyDataset
	return PyDataset(LoadDatasetCpp(url, cache_dir))

def LoadIdxDataset(url):
	return LoadDataset(url)
%}

%extend Visus::DbModule {
	static void attach() {

		//user defined attach
		DbModule::attach();

		//any time I want to create an IdxMultipleDataset, I create a PyMultipleDataset instead
		PrintInfo("Registering PyMultipleDataset");
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
%include <Visus/IdxFile.h>
%include <Visus/IdxHzOrder.h>
%include <Visus/Dataset.h>
%include <Visus/ModVisus.h>
%include <Visus/Db.h>

%include <Visus/IdxDataset.h>
%include <Visus/IdxDiskAccess.h>
%include <Visus/IdxMultipleDataset.h>
%include <Visus/VisusConvert.h>




