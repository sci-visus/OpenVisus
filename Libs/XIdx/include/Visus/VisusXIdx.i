%module(directors="1") VisusXIdxPy

%{
#include <Visus/Visus.h>
#include <Visus/Kernel.h>

#define SharedPtr std::shared_ptr

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

%shared_ptr(Visus::XIdxElement)
%shared_ptr(Visus::DataSource)
%shared_ptr(Visus::Attribute)
%shared_ptr(Visus::DataItem)
%shared_ptr(Visus::Variable)
%shared_ptr(Visus::Domain)
%shared_ptr(Visus::Group)
%shared_ptr(Visus::XIdxFile)
%shared_ptr(Visus::ListDomain)
%shared_ptr(Visus::HyperSlabDomain)
%shared_ptr(Visus::MultiAxisDomain)
%shared_ptr(Visus::Topology)
%shared_ptr(Visus::Geometry)
%shared_ptr(Visus::SpatialDomain)

%template(VectorOfXIdxElement) std::vector< SharedPtr< Visus::XIdxElement > >;
%template(VectorOfAttribute)   std::vector< SharedPtr< Visus::Attribute   > >;
%template(VectorOfDataItem)    std::vector< SharedPtr< Visus::DataItem    > >;
%template(VectorOfGroup)       std::vector< SharedPtr< Visus::Group       > >;
%template(VectorOfVariable)    std::vector< SharedPtr< Visus::Variable    > >;
%template(VectorOfDataSource)  std::vector< SharedPtr< Visus::DataSource  > >;

%include <Visus/Visus.h>
%include <Visus/Kernel.h>
%include <Visus/StringMap.h>
%include <Visus/Singleton.h>
%include <Visus/Object.h>

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
