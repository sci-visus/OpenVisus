%module(directors="1") VisusKernelPy

%{ 
#include <Visus/Array.h>
#include <Visus/VisusConfig.h>
using namespace Visus;
%}


%include <Visus/VisusPy.i>

%shared_ptr(Visus::HeapMemory)
%shared_ptr(Visus::DictObject)
%shared_ptr(Visus::ObjectCreator)
%shared_ptr(Visus::Object)
%shared_ptr(Visus::BoolObject)
%shared_ptr(Visus::IntObject)
%shared_ptr(Visus::Int64Object)
%shared_ptr(Visus::DoubleObject)
%shared_ptr(Visus::StringObject)
%shared_ptr(Visus::ListObject)
%shared_ptr(Visus::StringTree)
%shared_ptr(Visus::Array)
%shared_ptr(Visus::Color)
%shared_ptr(Visus::Box3<double>)
%shared_ptr(Visus::Box3<int>)
%shared_ptr(Visus::Box3<double>)
%shared_ptr(Visus::Box3<Visus::Int64>)
%shared_ptr(Visus::BoxN<double>)
%shared_ptr(Visus::BoxN<Visus::Int64>)
%shared_ptr(Visus::Matrix4)
%shared_ptr(Visus::Position)
%shared_ptr(Visus::Range)
%shared_ptr(Visus::DType)
%shared_ptr(Visus::Field)

%newobject Visus::ObjectStream::readObject;
%newobject Visus::ObjectCreator::createInstance;
%newobject Visus::ObjectFactory::createInstance;
%newobject Visus::StringTreeEncoder::encode;
%newobject Visus::StringTreeEncoder::decode;

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
   
      def __init__(self,object_name):
         ObjectCreator.__init__(self)
         self.object_name=object_name

      def createInstance(self):
         return eval(self.object_name+"()")

   ObjectFactory.getSingleton().registerObjectClass(object_name,object_name,MyObjectCreator(object_name))

%}

// _____________________________________________________
// extend Array
%extend Visus::Array {

Visus::Array operator[]  (int index)           const {return $self->getComponent(index);}
Visus::Array operator+   (Visus::Array& other) const {return ArrayUtils::add(*self,other);}
Visus::Array operator-   (Visus::Array& other) const {return ArrayUtils::sub(*self,other);}
Visus::Array operator*   (Visus::Array& other) const {return ArrayUtils::mul(*self,other);}
Visus::Array operator*   (double coeff)        const {return ArrayUtils::mul(*self,coeff);}
Visus::Array operator/   (Visus::Array& other) const {return ArrayUtils::div(*self,other);}
Visus::Array operator/   (double coeff)        const {return ArrayUtils::div(*self,coeff);}
Visus::Array& operator+= (Visus::Array& other)       {*self=ArrayUtils::add(*self,other); return *self;}
Visus::Array& operator-= (Visus::Array& other)       {*self=ArrayUtils::sub(*self,other); return *self;}
Visus::Array& operator*= (Visus::Array& other)       {*self=ArrayUtils::mul(*self,other); return *self;}
Visus::Array& operator*= (double coeff)              {*self=ArrayUtils::mul(*self,coeff); return *self;}
Visus::Array& operator/= (Visus::Array& other)       {*self=ArrayUtils::div(*self,other); return *self;} 
Visus::Array& operator/= (double coeff)              {*self=ArrayUtils::div(*self,coeff); return *self;}

%pythoncode %{
   
   # ////////////////////////////////////////////////////////
   def __rmul__(self, v):
      return self.__mul__(v)

   # ////////////////////////////////////////////////////////
   def toNumPy(src,bShareMem=False):
      import numpy
      shape=list(reversed([src.dims[I] for I in range(src.dims.getPointDim())]))
      shape.append(src.dtype.ncomponents())
      single_dtype=src.dtype.get(0).toString()
      if   single_dtype=="int8":     typestr="|i1"
      elif single_dtype=="uint8":    typestr="|u1"
      elif single_dtype=="in16":     typestr="<i2"
      elif single_dtype=="uint16":   typestr="<u2"
      elif single_dtype=="int32":    typestr="<i4"
      elif single_dtype=="uint32":   typestr="<u4"
      elif single_dtype=="float32":  typestr="<f4"
      elif single_dtype=="float64":  typestr="<f8"
      class numpy_holder(object): pass
      holder = numpy_holder()
      holder.__array_interface__ = {'strides': None,'shape': tuple(shape), 'typestr': typestr, 'data': (int(src.c_address()), False), 'version': 3 }
      ret = numpy.array(holder, copy=not bShareMem) 
      return ret
   
   # ////////////////////////////////////////////////////////
   def fromNumPy(src,bShareMem=False):
      import numpy
      if src.__array_interface__["strides"] is not None: raise Exception("numpy array is not memory contiguous")
      shape=src.__array_interface__["shape"]
      shape=tuple(reversed(shape))
      dims=NdPoint.one(len(shape))
      for I in range(dims.getPointDim()): dims.set(I,shape[I])   
      typestr=src.__array_interface__["typestr"]
      if   typestr=="|i1": dtype=DType.fromString("int8")
      elif typestr=="|u1": dtype=DType.fromString("uint8")
      elif typestr=="<i2": dtype=DType.fromString("in16")
      elif typestr=="<u2": dtype=DType.fromString("uint16")
      elif typestr=="<i4": dtype=DType.fromString("int32")
      elif typestr=="<u4": dtype=DType.fromString("uint32")
      elif typestr=="<f4": dtype=DType.fromString("float32")
      elif typestr=="<f8": dtype=DType.fromString("float64")
      else: raise Exception("unsupported typestr {}".format(typestr))
      c_address=str(src.__array_interface__["data"][0])
      return Array(dims,dtype,c_address,bShareMem)
   fromNumPy = staticmethod(fromNumPy)
%}
};

