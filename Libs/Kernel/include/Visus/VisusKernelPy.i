%module(directors="1") VisusKernelPy

%{ 
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
%include <Visus/ArrayUtils.h>
%include <Visus/ArrayPlugin.h>

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
  Visus::Array operator[](int index) const          {return $self->getComponent(index);}
  Visus::Array operator+(Visus::Array& other) const {return ArrayUtils::add(*self,other);}
  Visus::Array operator-(Visus::Array& other) const {return ArrayUtils::sub(*self,other);}
  Visus::Array operator*(Visus::Array& other) const {return ArrayUtils::mul(*self,other);}
  Visus::Array operator*(double coeff) const        {return ArrayUtils::mul(*self,coeff);}
  Visus::Array operator/(Visus::Array& other) const {return ArrayUtils::div(*self,other);}
  Visus::Array operator/(double coeff) const        {return ArrayUtils::div(*self,coeff);}
  Visus::Array& operator+=(Visus::Array& other)     {*self=ArrayUtils::add(*self,other); return *self;}
  Visus::Array& operator-=(Visus::Array& other)     {*self=ArrayUtils::sub(*self,other); return *self;}
  Visus::Array& operator*=(Visus::Array& other)     {*self=ArrayUtils::mul(*self,other); return *self;}
  Visus::Array& operator*=(double coeff)            {*self=ArrayUtils::mul(*self,coeff); return *self;}
  Visus::Array& operator/=(Visus::Array& other)     {*self=ArrayUtils::div(*self,other); return *self;} 
  Visus::Array& operator/=(double coeff)            {*self=ArrayUtils::div(*self,coeff); return *self;}
  %pythoncode %{
    def __rmul__(self, v):
      return self.__mul__(v)
  %}
};

