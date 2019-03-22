%module(directors="1") VisusKernelPy

%{ 
#include <Visus/Array.h>
#include <Visus/VisusConfig.h>
using namespace Visus;
%}

//Returning a pointer or reference in a director method is not recommended
%warnfilter(473) Visus::NodeCreator;


%include <Visus/VisusPy.i>

%shared_ptr(Visus::HeapMemory)

//VISUS_NEWOBJECT
//%newobject Visus::ClassName::MethodName;

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
//%apply SWIGTYPE *DISOWN              { Visus::ClassName*         disown};
//%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::DirectorClassName* disown};

%template(VectorOfField) std::vector<Visus::Field>;
%template(VectorOfArray) std::vector<Visus::Array>;

%include <Visus/Visus.h>
%include <Visus/Kernel.h>
%include <Visus/StringMap.h>
%include <Visus/Log.h>
%include <Visus/HeapMemory.h>
%include <Visus/Singleton.h>
%include <Visus/ObjectStream.h>
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
      shape=list(reversed([src.dims[I] for I in range(src.dims.getPointDim())]))
      shape.append(src.dtype.ncomponents())
      if bSqueeze: shape=[it for it in shape if it>1]
      single_dtype=src.dtype.get(0)
      typestr=("|" if single_dtype.getBitSize()==8 else "<") + ("f" if single_dtype.isDecimal() else ("u" if single_dtype.isUnsigned() else "i")) + str(int(single_dtype.getBitSize()/8))
      class numpy_holder(object): pass
      holder = numpy_holder()
      holder.__array_interface__ = {'strides': None,'shape': tuple(shape), 'typestr': typestr, 'data': (int(src.c_address()), False), 'version': 3 }
      ret = numpy.array(holder, copy=not bShareMem) 
      return ret
   toNumPy = staticmethod(toNumPy)
   
   # ////////////////////////////////////////////////////////
   def fromNumPy(src,TargetDim=0,bShareMem=False):
      import numpy
      if src.__array_interface__["strides"] is not None: 
        if bShareMem: raise Exception("numpy array is not memory contiguous","src.__array_interface__['strides']",src.__array_interface__["strides"],"bShareMem",bShareMem)
        src=numpy.ascontiguousarray(src)
      shape=src.__array_interface__["shape"]
      shape=tuple(reversed(shape))
      dims=NdPoint.one(len(shape))
      for I in range(dims.getPointDim()): dims.set(I,shape[I])   
      typestr=src.__array_interface__["typestr"]
      dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)
      c_address=str(src.__array_interface__["data"][0])
      ret=Array(dims,dtype,c_address,bShareMem)
      if TargetDim: 
        dims=NdPoint.one(TargetDim)
        for I in range(TargetDim): dims.set(I,ret.dims[ret.dims.getPointDim()-TargetDim+I])
        ret.resize(dims,DType(int(ret.dims.innerProduct()/dims.innerProduct()),ret.dtype), "Array::fromNumPy",0)
      return ret
   fromNumPy = staticmethod(fromNumPy)
%}
}; //%extend Visus::Array {


