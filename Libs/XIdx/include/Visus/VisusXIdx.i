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
ENABLE_SHARED_PTR(XIdxFile)
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

%inline %{
template<typename T>
Visus::SharedPtr<T>* toNewSharedPtr(Visus::SharedPtr<Domain> dom) {
  return dom ? new Visus::SharedPtr<T>(std::dynamic_pointer_cast<T>(dom)) : 0;
}
%}

%typemap(out) Visus::SharedPtr<Visus::Domain> Visus::Group::getDomain {
  std::string lookup_typename = result->type.toString();

  if(lookup_typename=="List"){
    lookup_typename = "_p_Visus__SharedPtrT_Visus__ListDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = toNewSharedPtr<ListDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  } else if(lookup_typename=="Spatial"){
    lookup_typename = "_p_Visus__SharedPtrT_Visus__SpatialDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = toNewSharedPtr<SpatialDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }
  else if(lookup_typename=="MultiAxisDomain"){
    lookup_typename = "_p_Visus__SharedPtrT_Visus__MultiAxisDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = toNewSharedPtr<MultiAxisDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }
  else if(lookup_typename=="HyperSlabDomain"){
    lookup_typename = "_p_Visus__SharedPtrT_Visus__HyperSlabDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = toNewSharedPtr<HyperSlabDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }

}

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
