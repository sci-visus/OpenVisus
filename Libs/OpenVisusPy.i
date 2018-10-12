%module(directors="1") OpenVisusPy

//common code
%begin %{
  #if _WIN32
    //this is needed if you dont' have python debug library
    //#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
    #pragma warning(disable: 4101)
    #pragma warning(disable: 4244)
  #endif
%}

%{
#define SWIG_FILE_WITH_INIT
%}

// Nested class not currently supported
#pragma SWIG nowarn=325 

// The 'using' keyword in template aliasing is not fully supported yet.
#pragma SWIG nowarn=342 

// Nested struct not currently supported
#pragma SWIG nowarn=312 

// warning : Nothing known about base class 'PositionRValue'
#pragma SWIG nowarn=401 

namespace Visus {}
%apply long long  { Visus::Int64 };

%include <numpy.i>

//__________________________________________________________
//STL

%include <stl.i>
%include <std_pair.i>
%include <std_vector.i>
%include <std_deque.i>
%include <std_string.i>
%include <std_map.i>
%include <std_set.i>

%template(PairDoubleDouble)                std::pair<double,double>;
%template(PairIntDouble)                   std::pair<int,double>;
%template(VectorString)                    std::vector< std::string >;
%template(VectorInt)                       std::vector<int>;
%template(VectorDouble)                    std::vector<double>;
%template(VectorFloat)                     std::vector<float>;
%template(MapStringString)                 std::map< std::string , std::string >;

//__________________________________________________________
//RENAME

%rename(From)                              *::from;
%rename(To)                                *::to;
%rename(assign         )                   *::operator=;
%rename(__getitem__    )                   *::operator[](int) const;
%rename(__getitem_ref__)                   *::operator[](int);
%rename(__bool_op__)                       *::operator bool() ;
%rename(__structure_derefence_op__)        *::operator->();
%rename(__const_structure_derefence_op__)  *::operator->() const;
%rename(__indirection_op__)                *::operator*();
%rename(__const_indirection_op__)          *::operator*() const;
%rename(__add__)                           *::operator+;
%rename(__sub__)                           *::operator-; 
%rename(__neg__)                           *::operator-();  // Unary -
%rename(__mul__)                           *::operator*;
%rename(__div__)                           *::operator/;

//__________________________________________________________
// EXCEPTION



//whenever a C++ exception happens, I should 'forward' it to python
%exception 
{
  try { 
    $action
  } 
  catch (Swig::DirectorMethodException& e) {
	VisusInfo()<<"Error happened in swig director code: "<<e.what();
	VisusInfo()<<PythonEngine::getLastErrorMessage();
    SWIG_fail;
  }
  catch (std::exception& e) {
    VisusInfo()<<"Swig Catched std::exception: "<<e.what();
    SWIG_exception_fail(SWIG_SystemError, e.what() );
  }
  catch (...) {
    VisusInfo()<<"Swig Catched ... exception: unknown reason";
    SWIG_exception(SWIG_UnknownError, "Unknown exception");
  }

}

//allow using PyObject* as input/output
%typemap(in) PyObject* { 
  $1 = $input; 
} 

// This is for other functions that want to return a PyObject. 
%typemap(out) PyObject* { 
  $result = $1; 
} 

//__________________________________________________________
// DISOWN
// grep for disown

//trick to simpligy the application of disown, rename the argument to disown
#define VISUS_DISOWN(argname) disown

// note: swig-defined DISOWN typemap crashes using directors 
// %apply SWIGTYPE *DISOWN { Visus::Node* disown }; THIS IS WRONG FOR DIRECTORS!
// see http://swig.10945.n7.nabble.com/Disown-Typemap-and-Directors-td9146.html)
%typemap(in, noblock=1) SWIGTYPE *DISOWN_FOR_DIRECTOR(int res = 0) 
{
  // BEGIN typemap DISOWN_FOR_DIRECTOR
  res = SWIG_ConvertPtr($input, %as_voidptrptr(&$1), $descriptor, SWIG_POINTER_DISOWN | %convertptr_flags);
  if (!SWIG_IsOK(res))  {
    %argument_fail(res,"$type", $symname, $argnum);
  }
  if (Swig::Director *director = dynamic_cast<Swig::Director *>($1)) 
    director->swig_disown(); //C++ will own swig counterpart
  // END typemap
}

//__________________________________________________________
// NEW_OBJECT

// IMPORTANT avoid returning director objects x
#define VISUS_NEWOBJECT(typename) typename

//__________________________________________________________
// SharedPtr
// grep -E -r -i -o -h --include *.h "SharedPtr<([[:space:]])*([[:alnum:]]+)([[:space:]]*)>" src  | sort -u

namespace Visus {
template<class T>
class SharedPtr 
{
public:
  SharedPtr();
  explicit SharedPtr(const SharedPtr& other);
  explicit SharedPtr(T* VISUS_DISOWN(ptr));
  ~SharedPtr();
  SharedPtr& operator=(const SharedPtr& other);
  void reset();
  void reset(T* VISUS_DISOWN(other));
  T* get() const;
  T& operator*() const;
  operator bool() const;
};

} //namespace Visus 

%define ENABLE_SHARED_PTR(ClassName) 
  %apply SWIGTYPE* DISOWN {Visus::ClassName* disown};
  %template(ClassName##Ptr) Visus::SharedPtr< Visus::ClassName >;
%enddef

//__________________________________________________________
//UniquePtr
// (not exposing UniquePtr to swig, please move it to the private section) ***


//__________________________________________________________
// ScopedVector
// (not exposing ScopedVector to swig, please move it to the private section) ***
//  %newobject Visus::ScopedVector::release;
// Visus::ScopedVector::*(...VISUS_DISOWN....)

//__________________________________________________________
%{ 

//Kernel
	#include <Visus/Visus.h>
	#include <Visus/PythonEngine.h>
	#include <Visus/Array.h>
	#include <Visus/VisusConfig.h>

//Dataflow
	#include <Visus/DataflowModule.h>
	#include <Visus/DataflowMessage.h>
	#include <Visus/DataflowPort.h>
	#include <Visus/DataflowNode.h>
	#include <Visus/Dataflow.h>

//Db
	#include <Visus/Db.h>
	#include <Visus/Access.h>
	#include <Visus/BlockQuery.h>
	#include <Visus/Query.h>

//Idx
	#include <Visus/Idx.h>
	#include <Visus/IdxFile.h>
	#include <Visus/IdxDataset.h>
	#include <Visus/IdxMultipleDataset.h>

//Nodes
	#include <Visus/Nodes.h>
	

#if VISUS_GUI

//Gui
	#include <Visus/Gui.h>
	#include <Visus/GLCanvas.h>
	namespace Visus { QWidget* ToCppQtWidget(PyObject* obj) {return obj? (QWidget*)PyLong_AsVoidPtr(obj) : nullptr; }}

//GuiNodes
	#include <Visus/PythonNode.h>

//AppKit
	#include <Visus/Viewer.h>

#endif 

using namespace Visus;



//deleteNumPyArrayCapsule
static void deleteNumPyArrayCapsule(PyObject *capsule)
{
	SharedPtr<HeapMemory>* keep_heap_in_memory=(SharedPtr<HeapMemory>*)PyCapsule_GetPointer(capsule, NULL);
	delete keep_heap_in_memory;
}

%}

//Kernel
	%include <Visus/Visus.h>
	%include <Visus/Kernel.h>
	%include <Visus/StringMap.h>
	%include <Visus/Log.h>
	ENABLE_SHARED_PTR(HeapMemory)
	%include <Visus/HeapMemory.h>
	%include <Visus/Singleton.h>
	%newobject Visus::ObjectStream::readObject;
	%newobject Visus::ObjectCreator::createInstance;
	%newobject Visus::ObjectFactory::createInstance;
	ENABLE_SHARED_PTR(DictObject)
	ENABLE_SHARED_PTR(ObjectCreator)
	ENABLE_SHARED_PTR(Object)
	%include <Visus/Object.h>
	%template(BoolPtr) Visus::SharedPtr<bool>;
	%include <Visus/Aborted.h>
	%newobject Visus::StringTreeEncoder::encode;
	%newobject Visus::StringTreeEncoder::decode;
	ENABLE_SHARED_PTR(StringTree)
	%include <Visus/StringTree.h>
	%include <Visus/VisusConfig.h>
	%include <Visus/Color.h>
	%include <Visus/Point.h> 
	  %template(Point2i)    Visus::Point2<int   > ;
	  %template(Point2f)    Visus::Point2<float > ;
	  %template(Point2d)    Visus::Point2<double> ;
	  %template(Point3i)    Visus::Point3<int  >  ;
	  %template(Point3f)    Visus::Point3<float>  ;
	  %template(Point3d)    Visus::Point3<double> ;
	  %template(Point4i)    Visus::Point4<int   > ;
	  %template(Point4f)    Visus::Point4<float > ;
	  %template(Point4d)    Visus::Point4<double> ;
	  %template(PointNi)    Visus::PointN<int >   ;
	  %template(PointNf)    Visus::PointN<float > ;
	  %template(PointNd)    Visus::PointN<double> ;
	  %template(NdPoint)    Visus::PointN< Visus::Int64 > ;
	%include <Visus/Box.h>
	  %template(Box3d)    Visus::Box3<double> ;
	  %template(Box3i)    Visus::Box3<int> ;
	  %template(BoxNd)    Visus::BoxN<double> ;
	  %template(NdBox)    Visus::BoxN< Visus::Int64 >;
	%include <Visus/Matrix.h>
	%include <Visus/Position.h>
	%include <Visus/Range.h>
	%include <Visus/DType.h>
	%template(VectorOfField) std::vector<Visus::Field>;
	%include <Visus/Field.h>

	ENABLE_SHARED_PTR(Array)
	%template(VectorOfArray) std::vector<Visus::Array>;
	%include <Visus/Array.h>

//Db
	ENABLE_SHARED_PTR(Access)
	ENABLE_SHARED_PTR(BlockQuery)
	ENABLE_SHARED_PTR(Query)
	ENABLE_SHARED_PTR(Dataset)
	%include <Visus/Db.h>
	%include <Visus/Access.h>
	%include <Visus/LogicBox.h>
	%include <Visus/BlockQuery.h>
	%include <Visus/Query.h>
	%include <Visus/DatasetBitmask.h>
	%include <Visus/Dataset.h>

//Dataflow
	ENABLE_SHARED_PTR(Dataflow)
	ENABLE_SHARED_PTR(DataflowMessage)
	ENABLE_SHARED_PTR(ReturnReceipt)
	ENABLE_SHARED_PTR(NodeJob)
	%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };
	%template(VectorNode) std::vector<Visus::Node*>;
	%feature("director") Visus::Node;
	%include <Visus/DataflowModule.h>
	%include <Visus/DataflowMessage.h>
	%include <Visus/DataflowPort.h>
	%include <Visus/DataflowNode.h>
	%include <Visus/Dataflow.h>

//Nodes
	%include <Visus/Nodes.h>

//Idx
	%include <Visus/Idx.h>
	%include <Visus/IdxFile.h>
	%include <Visus/IdxDataset.h>
	%include <Visus/IdxMultipleDataset.h>

//Gui
#if VISUS_GUI

//Gui
	#define Q_OBJECT
	#define signals public
	#define slots   
	%include <Visus/Gui.h>
	%include <Visus/GLObject.h>
	%include <Visus/GLMesh.h>
	%include <Visus/GLObjects.h>
	%include <Visus/GLCanvas.h>
	//see https://github.com/bleepbloop/Pivy/blob/master/interfaces/soqt.i
	namespace Visus {QWidget* ToCppQtWidget(PyObject* obj);}

//GuiNodes
	%include <Visus/GuiNodes.h>
	//allow rendering inside main GLCanvas (problem of multi-inheritance Node and GLObject)
	%feature("director") Visus::PythonNode; 
	%feature("nodirector") Visus::PythonNode::beginUpdate;
	%feature("nodirector") Visus::PythonNode::endUpdate;
	%feature("nodirector") Visus::PythonNode::getNodeBounds;
	%feature("nodirector") Visus::PythonNode::glSetRenderQueue;
	%feature("nodirector") Visus::PythonNode::glGetRenderQueue;
	%feature("nodirector") Visus::PythonNode::modelChanged;
	%feature("nodirector") Visus::PythonNode::enterInDataflow;
	%feature("nodirector") Visus::PythonNode::exitFromDataflow;
	%feature("nodirector") Visus::PythonNode::addNodeJob;
	%feature("nodirector") Visus::PythonNode::abortProcessing;
	%feature("nodirector") Visus::PythonNode::joinProcessing;
	%feature("nodirector") Visus::PythonNode::messageHasBeenPublished;
	%feature("nodirector") Visus::PythonNode::toBool;
	%feature("nodirector") Visus::PythonNode::toInt;
	%feature("nodirector") Visus::PythonNode::toInt64;
	%feature("nodirector") Visus::PythonNode::toDouble;
	%feature("nodirector") Visus::PythonNode::toString;
	%feature("nodirector") Visus::PythonNode::writeToObjectStream;
	%feature("nodirector") Visus::PythonNode::readFromObjectStream;
	%feature("nodirector") Visus::PythonNode::writeToSceneObjectStream;
	%feature("nodirector") Visus::PythonNode::readFromSceneObjectStream;
	%include <Visus/PythonNode.h>

//AppKit
	%include <Visus/AppKit.h>
	%include <Visus/Viewer.h>

#endif //#if VISUS_GUI

// _____________________________________________________
// init code 
%init %{

	//numpy does not work in windows/debug
	#if WIN32
		#if !defined(_DEBUG) || defined(SWIG_PYTHON_INTERPRETER_NO_DEBUG)
			import_array();
		#endif
	#else
		import_array();
	#endif
%}


// _____________________________________________________
// python code 

%pythoncode %{

# equivalent to VISUS_REGISTER_OBJECT_CLASS 
def VISUS_REGISTER_PYTHON_OBJECT_CLASS(object_name):

  class MyObjectCreator(ObjectCreator):

    # __init__
    def __init__(self,object_name):
      ObjectCreator.__init__(self)
      self.object_name=object_name

    # createInstance
    def createInstance(self):
      return eval(self.object_name+"()")

  ObjectFactory.getSingleton().registerObjectClass(object_name,object_name,MyObjectCreator(object_name))

%}


// _____________________________________________________
// extend Array



%extend Visus::Array {

  //operator[]
  Visus::Array operator[](int index) const {return $self->getComponent(index);}
  Visus::Array operator+(Visus::Array& other) const {return ArrayUtils::add(*self,other);}
  Visus::Array operator-(Visus::Array& other) const {return ArrayUtils::sub(*self,other);}
  Visus::Array operator*(Visus::Array& other) const {return ArrayUtils::mul(*self,other);}
  Visus::Array operator*(double coeff) const {return ArrayUtils::mul(*self,coeff);}
  Visus::Array operator/(Visus::Array& other) const {return ArrayUtils::div(*self,other);}
  Visus::Array operator/(double coeff) const {return ArrayUtils::div(*self,coeff);}
  Visus::Array& operator+=(Visus::Array& other)  {*self=ArrayUtils::add(*self,other); return *self;}
  Visus::Array& operator-=(Visus::Array& other)  {*self=ArrayUtils::sub(*self,other); return *self;}
  Visus::Array& operator*=(Visus::Array& other)  {*self=ArrayUtils::mul(*self,other); return *self;}
  Visus::Array& operator*=(double coeff) {*self=ArrayUtils::mul(*self,coeff); return *self;}
  Visus::Array& operator/=(Visus::Array& other)  {*self=ArrayUtils::div(*self,other); return *self;} 
  Visus::Array& operator/=(double coeff)  {*self=ArrayUtils::div(*self,coeff); return *self;}

  static Visus::Array fromVectorInt32(Visus::NdPoint dims, const std::vector<Visus::Int32>& vector) {
	return Visus::Array::fromVector<Visus::Int32>(dims, Visus::DTypes::INT32, vector);
  }

  static Visus::Array fromVectorFloat64(Visus::NdPoint dims, const std::vector<Visus::Float64>& vector) {
    return Visus::Array::fromVector<Visus::Float64>(dims, Visus::DTypes::FLOAT64, vector);
  }

  //asNumPy (the returned numpy will share the memory... see capsule code)
  PyObject* asNumPy() const
  {
    //in numpy the first dimension is the "upper dimension"
    //example:
    // a=array([[1,2,3],[4,5,6]])
    // print a.shape # return 2,3
    // print a[1,1]  # equivalent to print a[Y,X], return 5  
    npy_intp shape[VISUS_NDPOINT_DIM+1]; 
	int shape_dim=0;
    for (int I=VISUS_NDPOINT_DIM-1;I>=0;I--) {
		if ($self->dims[I]>1) 
		  shape[shape_dim++]=(npy_int)$self->dims[I];
    }

    int   ndtype        = $self->dtype.ncomponents();
    DType single_dtype  = $self->dtype.get(0);
    if (ndtype>1) shape[shape_dim++]=(npy_int)ndtype;
    int typenum;

    if      (single_dtype==(DTypes::UINT8  )) typenum=NPY_UINT8 ;
    else if (single_dtype==(DTypes::INT8   )) typenum=NPY_INT8  ;
    else if (single_dtype==(DTypes::UINT16 )) typenum=NPY_UINT16;
    else if (single_dtype==(DTypes::INT16  )) typenum=NPY_INT16 ;
    else if (single_dtype==(DTypes::UINT32 )) typenum=NPY_UINT32;
    else if (single_dtype==(DTypes::INT32  )) typenum=NPY_INT32 ;
    else if (single_dtype==(DTypes::UINT64 )) typenum=NPY_UINT64;
    else if (single_dtype==(DTypes::INT64  )) typenum=NPY_INT64 ;
    else if (single_dtype==(DTypes::FLOAT32)) typenum=NPY_FLOAT ;
    else if (single_dtype==(DTypes::FLOAT64)) typenum=NPY_DOUBLE;
    else {
		VisusInfo()<<"numpy type not supported "<<$self->dtype.toString();
		return nullptr;
	}

	//http://blog.enthought.com/python/numpy-arrays-with-pre-allocated-memory/#.W79FS-gzZtM
	//https://gitlab.onelab.info/gmsh/gmsh/commit/9cb49a8c372d2f7a48ee91ad2ca01c70f3b7cddf
	//NOTE: from documentatin: "...If data is passed to  PyArray_New, this memory must not be deallocated until the new array is deleted"
	auto data=(void*)$self->c_ptr();
	PyObject *ret = PyArray_New(&PyArray_Type, shape_dim, shape, typenum, NULL,data , 0, NPY_ARRAY_C_CONTIGUOUS | NPY_ARRAY_ALIGNED | NPY_ARRAY_WRITEABLE, NULL);
    PyArray_SetBaseObject((PyArrayObject*)ret, PyCapsule_New(new SharedPtr<HeapMemory>($self->heap), NULL, deleteNumPyArrayCapsule));
	return ret;
  }
  
  //constructor
  static Array fromNumPy(PyObject* obj)
  {
    if (!obj || !PyArray_Check((PyArrayObject*)obj)) 
	{
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"input argument is not a numpy array\n");
	  return Array();
    }
    
    PyArrayObject* numpy_array = (PyArrayObject*) obj;

	//must be contiguos
    if (!PyArray_ISCONTIGUOUS(numpy_array)) {
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"numpy array is null or not contiguous\n");
      return Array();
    }

    Uint8* c_ptr=(Uint8*)numpy_array->data;
    
    NdPoint dims=NdPoint::one(numpy_array->nd);
    for (int I=0;I<numpy_array->nd;I++)
      dims[I]=numpy_array->dimensions[numpy_array->nd-1-I];

    DType dtype;
    if      (PyArray_TYPE(numpy_array)==NPY_UINT8  ) dtype=(DTypes::UINT8  );
    else if (PyArray_TYPE(numpy_array)==NPY_INT8   ) dtype=(DTypes::INT8   );
    else if (PyArray_TYPE(numpy_array)==NPY_UINT16 ) dtype=(DTypes::UINT16 );
    else if (PyArray_TYPE(numpy_array)==NPY_INT16  ) dtype=(DTypes::INT16  );
    else if (PyArray_TYPE(numpy_array)==NPY_UINT32 ) dtype=(DTypes::UINT32 );
    else if (PyArray_TYPE(numpy_array)==NPY_INT32  ) dtype=(DTypes::INT32  );
    else if (PyArray_TYPE(numpy_array)==NPY_UINT64 ) dtype=(DTypes::UINT64 );
    else if (PyArray_TYPE(numpy_array)==NPY_INT64  ) dtype=(DTypes::INT64  );
    else if (PyArray_TYPE(numpy_array)==NPY_FLOAT  ) dtype=(DTypes::FLOAT32);
    else if (PyArray_TYPE(numpy_array)==NPY_DOUBLE ) dtype=(DTypes::FLOAT64);
    else {
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"cannot guess visus dtype from numpy array_type\n");
      return Array();
    }

	//cannot share the memory
	if (!(numpy_array->flags & NPY_OWNDATA) || PyArray_BASE(numpy_array)!=NULL)
	{
	  //Array ret(dims,dtype);
	  //memcpy(ret.c_ptr(),,ret.c_size());
	  //return ret;
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"numpy cannot share its internal memory\n");
      return Array();
	}

	auto heap=HeapMemory::createManaged((Uint8*)PyArray_DATA(numpy_array),dtype.getByteSize(dims));
	numpy_array->flags &= ~NPY_OWNDATA; //Heap has taken the property
	PyArray_SetBaseObject((PyArrayObject*)numpy_array, PyCapsule_New(new SharedPtr<HeapMemory>(heap), NULL, deleteNumPyArrayCapsule));

	return Array(dims,dtype,heap);
  }

  %pythoncode %{
    def __rmul__(self, v):
      return self.__mul__(v)
  %}
};

