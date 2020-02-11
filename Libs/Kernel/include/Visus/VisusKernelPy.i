%module(directors="1") VisusKernelPy

%{ 
#include <Visus/Array.h>
#include <Visus/StringTree.h>
#include <Visus/NetServer.h>
#include <Visus/Polygon.h>
#include <Visus/File.h>
#include <Visus/Time.h>
#include <Visus/TransferFunction.h>
#include <Visus/StringTree.h>
#include <Visus/Ray.h>
#include <Visus/Frustum.h>

using namespace Visus;
%}


%include <Visus/VisusPy.i>

%shared_ptr(Visus::HeapMemory)
%shared_ptr(Visus::StringTree)
%shared_ptr(Visus::ConfigFile)

//VISUS_NEWOBJECT (%use newobject or %newobject_director)
//%newobject Visus::ClassName::MethodName;
//%newobject_director(Visus::ClassName *, Visus::ClassName::MethodName);

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
//%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::DirectorClassName* disown};

%apply SWIGTYPE *DISOWN                { Visus::NetServerModule* disown};

%template(VectorInt)                       std::vector<int>;
%template(VectorDouble)                    std::vector<double>;
%template(VectorFloat)                     std::vector<float>;
%template(VectorString)                    std::vector< std::string >;
%template(PairDoubleDouble)                std::pair<double,double>;
%template(PairIntDouble)                   std::pair<int,double>;
%template(MapStringString)                 std::map< std::string , std::string >;

%include <Visus/Kernel.h>
	%template(VectorInt64)                     std::vector<Visus::Int64>;
%include <Visus/StringMap.h>
%include <Visus/HeapMemory.h>
%include <Visus/Singleton.h>
%include <Visus/Aborted.h>
%include <Visus/StringTree.h>

%include <Visus/StringTree.h>
%include <Visus/Color.h>
%include <Visus/Point.h> 

%include <Visus/Path.h>
%include <Visus/File.h>
%include <Visus/Time.h>

   %template(Point2i)    Visus::Point2<Visus::Int64>;
   %template(Point2f)    Visus::Point2<float>;
   %template(Point2d)    Visus::Point2<double>;

   %template(Point3i)    Visus::Point3<Visus::Int64>;
   %template(Point3f)    Visus::Point3<float>;
   %template(Point3d)    Visus::Point3<double>;

   %template(Point4i)    Visus::Point4<Visus::Int64>;
   %template(Point4f)    Visus::Point4<float>;
   %template(Point4d)    Visus::Point4<double>;
   
   %template(PointNd)    Visus::PointN<double>;
   %template(PointNi)    Visus::PointN<Visus::Int64>;

%include <Visus/Box.h>
   %template(BoxNd)    Visus::BoxN<double>;
   %template(BoxNi)    Visus::BoxN<Visus::Int64>;


%include <Visus/Quaternion.h> 
%include <Visus/Matrix.h>
%include <Visus/Polygon.h>
%include <Visus/Position.h>
%include <Visus/Quaternion.h>
%include <Visus/Polygon.h>
   %template(VectorPoint2d)  std::vector<Visus::Point2d>;
  
%include <Visus/Ray.h>
%include <Visus/Rectangle.h>
%include <Visus/Frustum.h>

%include <Visus/Range.h>
%include <Visus/DType.h>
%include <Visus/Field.h>
	%template(VectorOfField) std::vector<Visus::Field>;
%include <Visus/Array.h>
	%template(VectorOfArray) std::vector<Visus::Array>;
%include <Visus/ArrayUtils.h>
%include <Visus/ArrayPlugin.h>

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
   def toNumPy(src,bShareMem=False,bSqueeze=False):
      import numpy
      if not src.dtype.valid(): return numpy.zeros(0) 
      N=src.dtype.ncomponents()
      dtype=src.dtype.get(0)
      shape=list(reversed([src.dims[I] for I in range(src.dims.getPointDim())]))
      if N>1 : shape.append(N)
      if bSqueeze: shape=[it for it in shape if it>1]
      typestr=("|" if dtype.getBitSize()==8 else "<") + ("f" if dtype.isDecimal() else ("u" if dtype.isUnsigned() else "i")) + str(int(dtype.getBitSize()/8))
      class numpy_holder(object): pass
      holder = numpy_holder()
      holder.__array_interface__ = {'strides': None,'shape': tuple(shape), 'typestr': typestr, 'data': (int(src.c_address()), False), 'version': 3 }
      ret = numpy.array(holder, copy=not bShareMem) 
      return ret
   toNumPy = staticmethod(toNumPy)
   
   # ////////////////////////////////////////////////////////
   def fromNumPy(src,TargetDim=0,bShareMem=False):
      import numpy
      if src.shape==(0,): return Array()
      if src.__array_interface__["strides"] is not None: 
        if bShareMem: raise Exception("numpy array is not memory contiguous")
        src=numpy.ascontiguousarray(src)
      shape=src.__array_interface__["shape"]
      shape=tuple(reversed(shape))
      dims=PointNi.one(len(shape))
      for I in range(dims.getPointDim()): dims.set(I,shape[I])   
      typestr=src.__array_interface__["typestr"]
      dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)
      c_address=str(src.__array_interface__["data"][0])
      ret=Array(dims,dtype,c_address,bShareMem)

      if TargetDim!=0: 
        # example (3,512,512) uint8 -> (512,512) uint8[3]
        dims=PointNi.one(TargetDim)
        for I in range(TargetDim): 
          dims.set(I,ret.dims[ret.dims.getPointDim()-TargetDim+I])

        ret.resize(dims,DType(int(ret.dims.innerProduct()/dims.innerProduct()),ret.dtype), "Array::fromNumPy",0)

      return ret
   fromNumPy = staticmethod(fromNumPy)
%}

}; //%extend Visus::Array {

	
%shared_ptr(Visus::TransferFunction)
%include <Visus/TransferFunction.h>

%include <Visus/NetServer.h>

