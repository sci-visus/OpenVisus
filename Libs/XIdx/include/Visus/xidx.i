

%module(directors="1")  xidx


//
//%ignore xidx::Parasable::deserialize(xmlNode*, xidx::Parsable*);
//%ignore xidx::Parasable::deserialize(xmlNode*, Parsable*);
//%ignore Domain::deserialize(xmlNode*, xidx::Parsable*);

%{ 
#define SWIG_FILE_WITH_INIT
//#define SWIGPYTHON_BUILTIN
#include "xidx.h"

using namespace xidx;

%}

%include exception.i
%exception {
  try {
    $action
  } catch (const std::exception &e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

// Nested class not currently supported
#pragma SWIG nowarn=325

// The 'using' keyword in template aliasing is not fully supported yet.
#pragma SWIG nowarn=342

// Nested struct not currently supported
#pragma SWIG nowarn=312

// warning : Nothing known about base class 'PositionRValue'
#pragma SWIG nowarn=401

%feature("director") xidx::Parsable;
%feature("director") xidx::Attribute;
//%feature("director") xidx::Domain;

//%ignore *::deserialize(xmlNodePtr, Parsable*);
//%ignore *::deserialize(xmlNode*, xidx::Parsable*);
//%ignore Domain::deserialize(xmlNodePtr, Parsable*);
//%ignore Domain::deserialize(xmlNode*, xidx::Parsable*);

//%pythoncode %{
//  class MyVectorIterator(object):
//  
//    def __init__(self, pointerToVector):
//      self.pointerToVector = pointerToVector
//      self.index = -1
//    
//    def next(self):
//      self.index += 1
//      if self.index < len(self.pointerToVector):
//        return self.pointerToVector[self.index]
//      else:
//        raise StopIteration
//%}
//
//%rename(__cpp_iterator) std::vector<xidx::Attribute>::iterator;
//%rename(__cpp_insert) std::vector<xidx::Attribute>::insert;
//
//%extend std::vector<xidx::Attribute> {
//%pythoncode {
//    def iterator(self):
//      return MyVectorIterator(self)
//    def insert(self, i, x):
//      if isinstance(i, int): # "insert" is used as if the vector is a Python list
//        _XidxPy.AttributeVector___cpp_insert(self, self.begin() + i, x)
//      else: # "insert" is used as if the vector is a native C++ container
//        return _XidxPy.AttributeVector___cpp_insert(self, i, x)
//  }
//}

//%include <defines.i>
%include <typemaps.i>
%include <std_shared_ptr.i>
%include <stl.i>
//%include <pointer.i>
%include <std_pair.i>
%include <std_vector.i>
%include <std_deque.i>
%include <std_string.i>
%include <std_map.i>
%include <std_set.i>

%template(AttributeVector) std::vector<std::shared_ptr<xidx::Attribute>>;
%template(VariableVector)  std::vector<std::shared_ptr<xidx::Variable>>;
%template(DataItemVector)  std::vector<std::shared_ptr<xidx::DataItem>>;

%inline %{
  template<typename T>
  std::shared_ptr<T>* GetNewDomainPtr(std::shared_ptr<xidx::Domain> dom) {
    return dom ? new std::shared_ptr<T>(std::dynamic_pointer_cast<T>(dom)) : 0;
  }

%}

%typemap(out) std::shared_ptr<xidx::Domain> xidx::Group::getDomain {
  std::string lookup_typename = result->getClassName();

  if(lookup_typename=="ListDomain"){
    lookup_typename = "_p_std__shared_ptrT_xidx__ListDomainT_double_t_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = GetNewDomainPtr<xidx::ListDomain<double>>($1);
    
//    std::shared_ptr<  xidx::ListDomain<double> > *smartresult = $1 ? new std::shared_ptr<  xidx::ListDomain<double> >(std::dynamic_pointer_cast<ListDomain<double>>($1)) : 0;
    
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }
  else if(lookup_typename=="MultiAxisDomain"){
    lookup_typename = "_p_std__shared_ptrT_xidx__MultiAxisDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = GetNewDomainPtr<xidx::MultiAxisDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }
  else if(lookup_typename=="SpatialDomain"){
    lookup_typename = "_p_std__shared_ptrT_xidx__SpatialDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = GetNewDomainPtr<xidx::SpatialDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }
  else if(lookup_typename=="HyperSlabDomain"){
    lookup_typename = "_p_std__shared_ptrT_xidx__HyperSlabDomain_t";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    auto smartresult = GetNewDomainPtr<xidx::HyperSlabDomain>($1);
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), outtype, SWIG_POINTER_OWN);
  }

}

//Shared Pointers
%shared_ptr(xidx::Parsable)
%shared_ptr(xidx::Group)
%shared_ptr(xidx::DataSource)
%shared_ptr(xidx::XidxFile)
%shared_ptr(xidx::Domain)
%shared_ptr(xidx::SpatialDomain)
%shared_ptr(xidx::MultiAxisDomain)
%shared_ptr(xidx::HyperSlabDomain)
%shared_ptr(xidx::ListDomain)
//%shared_ptr(std::vector<double>)
%shared_ptr(xidx::Attribute)
%shared_ptr(xidx::DataItem)
%shared_ptr(xidx::Variable)
%shared_ptr(xidx::ListDomain<double>)
//%shared_ptr(ListDomainDouble)

%include <xidx.h>
%include <xidx_file.h>
%include <xidx_data_source.h>
%include <xidx_index_space.h>
%include <elements/xidx_attribute.h>
%include <elements/xidx_dataitem.h>
%include <elements/xidx_domain.h>
%include <elements/xidx_types.h>
%include <elements/xidx_geometry.h>
%include <elements/xidx_topology.h>
%include <elements/xidx_spatial_domain.h>
%include <elements/xidx_list_domain.h>
%include <elements/xidx_hyperslab_domain.h>
%include <elements/xidx_multiaxis_domain.h>

%include <elements/xidx_group.h>
%include <elements/xidx_variable.h>
%include <elements/xidx_attribute.h>
%include <elements/xidx_parsable.h>
%include <elements/xidx_parse_utils.h>

//%rename(deserialize)                              *::deserialize;

%template(ListDomainDouble) xidx::ListDomain<double>;
%template(IndexSpace) std::vector<double>;
%template(GroupVector) std::vector<std::shared_ptr<xidx::Group>>;

%include "xidx.h"
%include "xidx_file.h"
%include "xidx_data_source.h"
%include "xidx_index_space.h"
%include "elements/xidx_attribute.h"
%include "elements/xidx_dataitem.h"
%include "elements/xidx_domain.h"
%include "elements/xidx_spatial_domain.h"
%include "elements/xidx_hyperslab_domain.h"
%include "elements/xidx_list_domain.h"
%include "elements/xidx_multiaxis_domain.h"
%include "elements/xidx_types.h"
%include "elements/xidx_geometry.h"
%include "elements/xidx_topology.h"
%include "elements/xidx_group.h"
%include "elements/xidx_variable.h"
%include "elements/xidx_attribute.h"
%include "elements/xidx_parsable.h"
%include "elements/xidx_parse_utils.h"
