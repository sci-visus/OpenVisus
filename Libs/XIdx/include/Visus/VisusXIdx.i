%module(directors="1") VisusXIdxPy

%{ 
#include <Visus/VisusXIdx.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>
%import <Visus/VisusKernelPy.i>

%include <Visus/VisusXIdx.h>

ENABLE_SHARED_PTR(XIdxElement)
ENABLE_SHARED_PTR(DataSource)
ENABLE_SHARED_PTR(Attribute)
ENABLE_SHARED_PTR(DataItem)
ENABLE_SHARED_PTR(Variable)
ENABLE_SHARED_PTR(Domain)
ENABLE_SHARED_PTR(Group)
ENABLE_SHARED_PTR(ListDomain)
ENABLE_SHARED_PTR(HyperSlabDomain)
ENABLE_SHARED_PTR(MultiAxisDomain)
ENABLE_SHARED_PTR(Topology)
ENABLE_SHARED_PTR(Geometry)
ENABLE_SHARED_PTR(SpatialDomain)

%template(VectorOfXIdxElement) std::vector< Visus::SharedPtr< Visus::XIdxElement > >;
%template(VectorOfAttribute)   std::vector< Visus::SharedPtr< Visus::Attribute   > >;
%template(VectorOfDataItem)    std::vector< Visus::SharedPtr< Visus::DataItem    > >;
%template(VectorOfGroup)       std::vector< Visus::SharedPtr< Visus::Group       > >;
%template(VectorOfVariable)    std::vector< Visus::SharedPtr< Visus::Variable    > >;
%template(VectorOfDataSource)  std::vector< Visus::SharedPtr< Visus::DataSource  > >;

%include <Visus/xidx_element.h>
%include <Visus/xidx_datasource.h>
%include <Visus/xidx_attribute.h>
%include <Visus/xidx_dataitem.h>
%include <Visus/xidx_variable.h>
%include <Visus/xidx_domain.h>
%include <Visus/xidx_group.h>

%include <Visus/xidx_list_domain.h>
%include <Visus/xidx_hyperslab_domain.h>
%include <Visus/xidx_multiaxis_domain.h>

%include <Visus/xidx_topology.h>
%include <Visus/xidx_geometry.h>
%include <Visus/xidx_spatial_domain.h>
