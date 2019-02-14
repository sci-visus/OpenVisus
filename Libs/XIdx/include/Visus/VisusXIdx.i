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
//%include <Visus/VisusPy.i>
//%import <Visus/VisusKernelPy.i>

typedef SharedPtr std::shared_ptr;
%typemap(SharedPtr) std::shared_ptr;


%include <Visus/VisusXIdx.h>


//template <class T> using Visus::SharedPtr = std::shared_ptr<T>;
//template <class T> using WeakPtr = std::weak_ptr  <T>;
//template <class T> using UniquePtr = std::unique_ptr<T>;

//ENABLE_SHARED_PTR(XIdxElement)
//ENABLE_SHARED_PTR(DataSource)
//ENABLE_SHARED_PTR(Attribute)
//ENABLE_SHARED_PTR(DataItem)
//ENABLE_SHARED_PTR(Variable)
//ENABLE_SHARED_PTR(Domain)
//ENABLE_SHARED_PTR(Group)
//ENABLE_SHARED_PTR(XIdxFile)
//ENABLE_SHARED_PTR(ListDomain)
//ENABLE_SHARED_PTR(HyperSlabDomain)
//ENABLE_SHARED_PTR(MultiAxisDomain)
//ENABLE_SHARED_PTR(Topology)
//ENABLE_SHARED_PTR(Geometry)
//ENABLE_SHARED_PTR(SpatialDomain)

%shared_ptr(Visus::Object)
%shared_ptr(Visus::BoolObject)
%shared_ptr(Visus::IntObject)
%shared_ptr(Visus::Int64Object)
%shared_ptr(Visus::DoubleObject)
%shared_ptr(Visus::StringObject)
%shared_ptr(Visus::ListObject)
%shared_ptr(Visus::DictObject)
%shared_ptr(Visus::ObjectEncoder)


%shared_ptr(XIdxElement)
%shared_ptr(DataSource)
%shared_ptr(Attribute)
%shared_ptr(DataItem)
%shared_ptr(Variable)
%shared_ptr(Domain)
%shared_ptr(Group)
%shared_ptr(XIdxFile)
%shared_ptr(ListDomain)
%shared_ptr(HyperSlabDomain)
%shared_ptr(MultiAxisDomain)
%shared_ptr(Topology)
%shared_ptr(Geometry)
%shared_ptr(SpatialDomain)

//%template(VectorOfXIdxElement) std::vector< Visus::SharedPtr< Visus::XIdxElement > >;
//%template(VectorOfAttribute)   std::vector< Visus::SharedPtr< Visus::Attribute   > >;
//%template(VectorOfDataItem)    std::vector< Visus::SharedPtr< Visus::DataItem    > >;
//%template(VectorOfGroup)       std::vector< Visus::SharedPtr< Visus::Group       > >;
//%template(VectorOfVariable)    std::vector< Visus::SharedPtr< Visus::Variable    > >;
//%template(VectorOfDataSource)  std::vector< Visus::SharedPtr< Visus::DataSource  > >;

%template(VectorOfXIdxElement) std::vector< std::shared_ptr< Visus::XIdxElement > >;
%template(VectorOfAttribute)   std::vector< std::shared_ptr< Visus::Attribute   > >;
%template(VectorOfDataItem)    std::vector< std::shared_ptr< Visus::DataItem    > >;
%template(VectorOfGroup)       std::vector< std::shared_ptr< Visus::Group       > >;
%template(VectorOfVariable)    std::vector< std::shared_ptr< Visus::Variable    > >;
%template(VectorOfDataSource)  std::vector< std::shared_ptr< Visus::DataSource  > >;

%inline %{
template<typename T>
std::shared_ptr<T>* toNewSharedPtr(std::shared_ptr<Domain> dom) {
  return dom ? new std::shared_ptr<T>(std::dynamic_pointer_cast<T>(dom)) : 0;
}

%}

/*
%typemap(out) std::shared_ptr<Visus::Domain> Visus::Group::getDomain {
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
*/
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
