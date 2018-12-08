%module(directors="1") VisusKernelPy

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/Array.h>
#include <Visus/VisusConfig.h>
using namespace Visus;

static char* __my_numpy_capsule_name__="__my_numpy_capsule_name__";

%}


%include <Visus/VisusPy.i>

%{ 

static SharedPtr<HeapMemory> getNumPyHeap(PyArrayObject *numpy_array)
{
	auto base=PyArray_BASE(numpy_array);
	if (!base)
		return SharedPtr<HeapMemory>();

	auto ptr=PyCapsule_GetPointer(base, __my_numpy_capsule_name__);
	if (!ptr)
		return SharedPtr<HeapMemory>();

	return *static_cast<SharedPtr<HeapMemory>*>(ptr);
}

//deleteNumPyCapsule
static void deleteNumPyCapsule(PyObject *capsule)
{
	auto ptr=(SharedPtr<HeapMemory>*)PyCapsule_GetPointer(capsule, __my_numpy_capsule_name__);
	VisusReleaseAssert(ptr);
	delete ptr;
}

static void setNumPyHeap(PyArrayObject* numpy_array,SharedPtr<HeapMemory> heap)
{
	VisusReleaseAssert(!PyArray_BASE(numpy_array));
	auto base=PyCapsule_New(new SharedPtr<HeapMemory>(heap), __my_numpy_capsule_name__, deleteNumPyCapsule);
	PyArray_SetBaseObject((PyArrayObject*)numpy_array,base);
}

%}

ENABLE_SHARED_PTR(HeapMemory)
ENABLE_SHARED_PTR(DictObject)
ENABLE_SHARED_PTR(ObjectCreator)
ENABLE_SHARED_PTR(Object)
ENABLE_SHARED_PTR(StringTree)
ENABLE_SHARED_PTR(Array)

%newobject Visus::ObjectStream::readObject;
%newobject Visus::ObjectCreator::createInstance;
%newobject Visus::ObjectFactory::createInstance;
%newobject Visus::StringTreeEncoder::encode;
%newobject Visus::StringTreeEncoder::decode;

%template(BoolPtr) Visus::SharedPtr<bool>;
%template(VectorOfField) std::vector<Visus::Field>;
%template(VectorOfArray) std::vector<Visus::Array>;

%include <Visus/Visus.h>
%include <Visus/Kernel.h>
%include <Visus/StringMap.h>
%include <Visus/Log.h>
%include <Visus/HeapMemory.h>
%include <Visus/Singleton.h>
%include <Visus/Object.h>
%include <Visus/Aborted.h>
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
%include <Visus/Field.h>
%include <Visus/Array.h>


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

  //toNumPy (the returned numpy will share the memory... see capsule code)
  PyObject* toNumPy(bool bSqueeze=true) const
  {
    //in numpy the first dimension is the "upper dimension"
    //example:
    // a=array([[1,2,3],[4,5,6]])
    // print a.shape # return 2,3
    // print a[1,1]  # equivalent to print a[Y,X], return 5  

    int   ncomponents   = $self->dtype.ncomponents();
    DType single_dtype  = $self->dtype.get(0);

	std::vector<npy_intp> shape;

    if (ncomponents>1) 
		shape.push_back((npy_int)ncomponents);

    for (int I=0,N=$self->dims.getPointDim();I<N;I++)
	{
		auto single_dim=(npy_int)$self->dims[I];
		if (!bSqueeze || single_dim>1)
			shape.push_back(single_dim);
	}

	std::reverse(shape.begin(), shape.end());

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
	//https://stackoverflow.com/questions/52731884/pyarray-simplenewfromdata
	PyObject *numpy_array = PyArray_SimpleNewFromData((int)shape.size(), &shape[0], typenum, (void*)$self->c_ptr());
	setNumPyHeap((PyArrayObject*)numpy_array,$self->heap);
	return numpy_array;
  }
  
  //fromNumPy (the returned numpy will share the memory if possible... see capsule code)
  static Array fromNumPy(PyObject* obj, int ndim=0)
  {
    if (!obj || !PyArray_Check((PyArrayObject*)obj)) 
	{
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"input argument is not a numpy array\n");
	  return Array();
    }
    
    PyArrayObject* numpy_array = (PyArrayObject*) obj;

	//must be contiguos
    if (!PyArray_ISCONTIGUOUS(numpy_array)) {
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"numpy array is not contiguous\n");
      return Array();
    }

    Uint8* c_ptr=(Uint8*)numpy_array->data;

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

	std::vector<int> shape;
    for (int I=0;I<numpy_array->nd;I++)
		shape.push_back(numpy_array->dimensions[I]);
	std::reverse(shape.begin(), shape.end());

	if (ndim==0)
		ndim=numpy_array->nd;

    NdPoint dims=NdPoint::one(ndim);

	while (shape.size()>ndim)
	{
		dtype=DType(shape[0] * dtype.ncomponents(),dtype.get(0)); 
		shape.erase(shape.begin());
	}

	for (int I=0;I<ndim;I++)
		dims[I]=I<shape.size()? shape[I] : 1;

	//already stolen the property
	if (auto heap=getNumPyHeap(numpy_array))
	{
		//note: this case has not been debugged
		VisusReleaseAssert(!(numpy_array->flags & NPY_OWNDATA));
		return Array(dims,dtype,heap);

	}
	//steal the property
	else if (!PyArray_BASE(numpy_array) && (numpy_array->flags & NPY_OWNDATA))
	{
		//steal the property
		auto heap=HeapMemory::createManaged((Uint8*)PyArray_DATA(numpy_array),dtype.getByteSize(dims));
		numpy_array->flags &= ~NPY_OWNDATA; 

		setNumPyHeap(numpy_array,heap);
		return Array(dims,dtype,heap);
	}
	else
	{
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"numpy cannot share its internal memory\n");
      return Array();
	}

  }

  %pythoncode %{
    def __rmul__(self, v):
      return self.__mul__(v)
  %}
};

