%module(directors="1") VisusXIdxPy

%{
#include <Visus/Visus.h>
#include <Visus/Kernel.h>
#include <Visus/VisusXIdx.h>
using namespace Visus;
%}

%include <std_shared_ptr.i>
%include <std_vector.i>
%include <std_string.i>
%include <typemaps.i>

%include <Visus/VisusPy.i>
%import <Visus/VisusKernelPy.i>

%include <Visus/VisusXIdx.h>

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN  { Visus::XIdxElement*      disown};
%apply SWIGTYPE *DISOWN  { Visus::DataSource*       disown};
%apply SWIGTYPE *DISOWN  { Visus::Attribute*        disown};
%apply SWIGTYPE *DISOWN  { Visus::DataItem*         disown};
%apply SWIGTYPE *DISOWN  { Visus::Variable*         disown};
%apply SWIGTYPE *DISOWN  { Visus::Domain*           disown};
%apply SWIGTYPE *DISOWN  { Visus::Group*            disown};
%apply SWIGTYPE *DISOWN  { Visus::XIdxFile*         disown};
%apply SWIGTYPE *DISOWN  { Visus::ListDomain*       disown};
%apply SWIGTYPE *DISOWN  { Visus::HyperSlabDomain*  disown};
%apply SWIGTYPE *DISOWN  { Visus::MultiAxisDomain*  disown};
%apply SWIGTYPE *DISOWN  { Visus::Topology*         disown};
%apply SWIGTYPE *DISOWN  { Visus::Geometry*         disown};
%apply SWIGTYPE *DISOWN  { Visus::SpatialDomain*    disown};

//VISUS_NEWOBJECT
%newobject Visus::Domain::createDomain;
%newobject Visus::IdxFile::load;
%newobject Visus::XIdxElement::readChild;

%template(VectorOfAttribute)       std::vector<Visus::Attribute*>;
%template(VectorOfDataItem)        std::vector<Visus::DataItem*>;
%template(VectorOfXIdxElement)     std::vector<Visus::XIdxElement*>;
%template(VectorOfGroup)           std::vector<Visus::Group*>;
%template(VectorOfVariable)        std::vector<Visus::Variable*>;
%template(VectorOfDataSource)      std::vector<Visus::DataSource*>;

%typemap(out) Visus::Domain* Visus::Group::getDomain 
{
	//scrgiorgio: i don't think the returned ptr should have a SWIG_OWN flag
  if(dynamic_cast<ListDomain*>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), SWIG_TypeQuery("Visus::ListDomain *"), 0 | 0);
  else if(dynamic_cast<SpatialDomain*>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), SWIG_TypeQuery("Visus::SpatialDomain *"), 0 | 0);
  else if(dynamic_cast<MultiAxisDomain*>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), SWIG_TypeQuery("Visus::MultiAxisDomain *"), 0 | 0);
  else if(dynamic_cast<HyperSlabDomain*>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), SWIG_TypeQuery("Visus::HyperSlabDomain *"), 0 | 0);
}

%include <Visus/Visus.h>
%include <Visus/Kernel.h>
%include <Visus/StringMap.h>
%include <Visus/Singleton.h>
%include <Visus/ObjectStream.h>

%include <Visus/xidx_element.h>
%include <Visus/xidx_datasource.h>
%include <Visus/xidx_attribute.h>
%include <Visus/xidx_dataitem.h>
%include <Visus/xidx_variable.h>
%include <Visus/xidx_domain.h>
%include <Visus/xidx_group.h>
%include <Visus/xidx_file.h>

%include <Visus/xidx_list_domain.h>
%include <Visus/xidx_hyperslab_domain.h>
%include <Visus/xidx_multiaxis_domain.h>

%include <Visus/xidx_topology.h>
%include <Visus/xidx_geometry.h>
%include <Visus/xidx_spatial_domain.h>
