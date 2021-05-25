%module(directors="1") VisusPy

%{ 
#include <Visus/Array.h>
#include <Visus/StringTree.h>
#include <Visus/NetServer.h>
#include <Visus/Polygon.h>
#include <Visus/File.h>
#include <Visus/Time.h>
#include <Visus/TransferFunction.h>
#include <Visus/Ray.h>
#include <Visus/Frustum.h>
#include <Visus/NetService.h>
#include <Visus/RamResource.h>

#include <Visus/Db.h>
#include <Visus/StringTree.h>
#include <Visus/Access.h>
#include <Visus/Query.h>
#include <Visus/BlockQuery.h>
#include <Visus/BoxQuery.h>
#include <Visus/PointQuery.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/Dataset.h>
#include <Visus/ModVisus.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/VisusConvert.h>
#include <PyMultipleDataset.h>
using namespace Visus;


using namespace Visus;
%}

%include <VisusPy.common>


%template(StdVectorInt)                       std::vector<int>;
%template(StdVectorDouble)                    std::vector<double>;
%template(StdVectorFloat)                     std::vector<float>;
%template(StdVectorStdString)                 std::vector< std::string >;
%template(StdPairDoubleDouble)                std::pair<double,double>;
%template(StdPairIntDouble)                   std::pair<int,double>;
%template(StdMapStdStringStdString)           std::map< std::string , std::string >;
%template(StdVectorLongLong)                  std::vector<long long>;

//%feature("director") Visus::PLESE_README_ME_CAREFULLY
/*
see https://github.com/swig/swig/issues/306)
Calling the C++ part of a director, swig does not release the GIL.
Say for example that the C++ will hold in a semaphore.down() waiting for a condition (but it has the GIL!)
Meanwhile some other thread which should signal that semaphroe needs the GIL but cannot get it
SOLUTION: use virtuals only for very needed methods, and exclude ( %feature("nodirector") Visus::ClassName:methodName;) all others

see https://blog.mbedded.ninja/programming/languages/python/python-swig-bindings-from-cplusplus/#cross-language-polymorphism-directors
Normally, you can pass in the -threads argument when calling SWIG to make SWIG release the GIL upon every entry to your C/C++ library from Python
(or if you are using CMake, call SET_PROPERTY(SOURCE MyInterfaceFile.i PROPERTY SWIG_FLAGS "-threads") ).
However, this is not the case for any C/C++ class that has been converted into a director.
*/


%shared_ptr(Visus::HeapMemory)
%shared_ptr(Visus::StringTree)
%shared_ptr(Visus::ConfigFile)

//VISUS_NEWOBJECT (%use newobject or %newobject_director)
//%newobject Visus::ClassName::MethodName;
//%newobject_director(Visus::ClassName *, Visus::ClassName::MethodName);

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
//%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::DirectorClassName* disown};

%apply SWIGTYPE *DISOWN                { Visus::NetServerModule* disown};



%include <Visus/Kernel.h>
	
%include <Visus/StringMap.h>
%include <Visus/HeapMemory.h>
%include <Visus/RamResource.h>
%include <Visus/Aborted.h>
%include <Visus/StringTree.h>
%include <Visus/Color.h>
%include <Visus/Path.h>
%include <Visus/File.h>
%include <Visus/Time.h>
%include <Visus/StringUtils.h>

%include <Visus/Point.h> 
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
   %template(BoxNd)        Visus::BoxN<double>;
   %template(BoxNi)        Visus::BoxN<Visus::Int64>;
   %template(VectorBoxNi)  std::vector<Visus::BoxNi>;

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
def toNumPy(src, bShareMem=False, bSqueeze=False):

	import numpy

	# invalid arrray is a zero numpy,0 is "shape"
	if not src.dtype.valid(): 
		return numpy.zeros(0, dtype=numpy.float) 

	# dtype  (<: little-endian, >: big-endian, |: not-relevant) ; integer providing the number of bytes  ; i (integer) u (unsigned integer) f (floating point)
	atomic_dtype=src.dtype.get(0)
	typestr="".join([
		"|" if atomic_dtype.getBitSize()==8 else "<",
		"f" if atomic_dtype.isDecimal() else ("u" if atomic_dtype.isUnsigned() else "i"),
		str(int(atomic_dtype.getBitSize()/8))
	])  

	# shape (can be multi components)
	shape=list(reversed([src.dims[I] for I in range(src.dims.getPointDim())]))
	if src.dtype.ncomponents()>1 : 
		shape.append(src.dtype.ncomponents())

	# no real data, just keep the "dimensions" of the data
	if 0 in shape:

		return numpy.zeros(shape, dtype=numpy.dtype(typestr))

	else:

		if bSqueeze: 
			shape=[it for it in shape if it>1]

		class MyNumPyHolder(object): 
			pass

		holder = MyNumPyHolder()
		  
		holder.__array_interface__ = {
			'strides': None,
			'shape': tuple(shape), 
			'typestr': typestr, 
			'data': (int(src.c_address()), False),  # The second entry in the tuple is a read-only flag (true means the data area is read-only).
			'version': 3 
		}

		return numpy.array(holder, copy=False if bShareMem else True) 

toNumPy = staticmethod(toNumPy)

# ////////////////////////////////////////////////////////
def fromNumPy(src, TargetDim=0, bShareMem=False):

	import numpy

	# is not memory contigous...
	bContiguos = src.__array_interface__["strides"] is None
	if not bContiguos: 
		  
		if bShareMem: 
			raise Exception("Cannot share memory since the original numpy array is not memory contiguous")
			
		src=numpy.ascontiguousarray(src)
		  
	# dtype
	typestr = src.__array_interface__["typestr"]
	dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)

	# shape (reversed!)
	shape   = src.__array_interface__["shape"]
	shape=tuple(reversed(shape)) 
	pdim=len(shape)
	dims=PointNi.one(pdim)
	for I in range(pdim):  
		dims.set(I,shape[I]) 

	if dims.innerProduct() == 0:
		ret=Array(dims, dtype)

	else:
		c_address=str(src.__array_interface__["data"][0]) # [0] element is the address
		ret=Array(dims, dtype, c_address, bShareMem)

	# example (3,512,512) uint8 TargetDim=2 -> (512,512) uint8[3]
	if TargetDim > 0 and ret.dims.getPointDim() > TargetDim:
		v=ret.dims.toVector()
		A=1
		for it in v[0:len(v)-TargetDim]: A*=it # first remaining elements
		B=PointNi(v[-TargetDim:]) # last 'TargetDim' elements
		ret.resize(B,DType(A,ret.dtype), "Array::fromNumPy",0)

	return ret

fromNumPy = staticmethod(fromNumPy)
%}

}; //%extend Visus::Array {


%include <Visus/NetMessage.h>
%include <Visus/NetSocket.h>
%include <Visus/NetService.h>
%include <Visus/NetServer.h>
	
%shared_ptr(Visus::TransferFunction)
%include <Visus/TransferFunction.h>

%include <Visus/NetServer.h>

#if VISUS_SLAM
	%{ 
	#include <slam.h>
	%}

	%feature("director") Visus::Slam;

	%apply SWIGTYPE* DISOWN {Visus::Camera* disown};

	%template(VectorOfCamera)     std::vector<Visus::Camera*>;
	%template(VectorOfMatch)      std::vector<Visus::Match>;
	%template(VectorOfKeyPoint)   std::vector<Visus::KeyPoint>;

	%include <slam.h>
#endif 

%pythoncode %{
def convert_dtype(value):

	import numpy

	# get first component
	if isinstance(value,DType):
		value=value.get(0).toString() 
	
	if isinstance(value,str):
		if value=="uint8":    return numpy.uint8
		if value=="int8":     return numpy.int8
		if value=="uint16":   return numpy.uint16
		if value=="int16":    return numpy.int16
		if value=="uint32":   return numpy.uint32
		if value=="int32":    return numpy.int32
		if value=="float32":  return numpy.float32
		if value=="float64":  return numpy.float64
			
	if isinstance(value,numpy):
		if value==numpy.uint8:   return "uint8"
		if value==numpy.int8:    return "int8"
		if value==numpy.uint16:  return "uint16"
		if value==numpy.int16:   return "int16"
		if value==numpy.uint32:  return "uint32"
		if value==numpy.int32:   return "int32"
		if value==numpy.float32: return "float32"
		if value==numpy.float64: return "float64"
		
	raise Exception("Internal error")
%}



///////////////////////////////////////////////////// DB part

%shared_ptr(Visus::Access)
%shared_ptr(Visus::IdxDiskAccess)
%shared_ptr(Visus::IdxMultipleAccess)

%shared_ptr(Visus::Query)
%shared_ptr(Visus::BlockQuery)
%shared_ptr(Visus::BoxQuery)
%shared_ptr(Visus::PointQuery)

%shared_ptr(Visus::Dataset)
%shared_ptr(Visus::GoogleMapsDataset)
%shared_ptr(Visus::IdxDataset)
%shared_ptr(Visus::IdxMultipleDataset)

%rename(LoadDatasetCpp) Visus::LoadDataset;

%typemap(out) std::shared_ptr<Visus::Dataset>  
{
  //Dataset typemape DSTMP01
  if(auto midx=std::dynamic_pointer_cast<IdxMultipleDataset>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  IdxMultipleDataset >(midx) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__IdxMultipleDataset_t, SWIG_POINTER_OWN);

  else if(auto idx=std::dynamic_pointer_cast<IdxDataset>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  IdxDataset >(idx) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__IdxDataset_t, SWIG_POINTER_OWN);

  else if(auto google=std::dynamic_pointer_cast<GoogleMapsDataset>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  GoogleMapsDataset >(google) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__GoogleMapsDataset_t, SWIG_POINTER_OWN);

  else
	$result= SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  Dataset >(result) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__Dataset_t, SWIG_POINTER_OWN);
}


%pythoncode %{
def LoadDataset(url):
	from OpenVisus.dataset import PyDataset
	return PyDataset(LoadDatasetCpp(url))

def LoadIdxDataset(url):
	return LoadDataset(url)
%}

%extend Visus::DbModule {
	static void attach() {

		//user defined attach
		DbModule::attach();

		//any time I want to create an IdxMultipleDataset, I create a PyMultipleDataset instead
		PrintInfo("Registering PyMultipleDataset");
		DatasetFactory::getSingleton()->registerDatasetType("IdxMultipleDataset", []() {return std::make_shared<PyMultipleDataset>(); });
	}
}
%ignore Visus::DbModule::attach;

%include <Visus/Db.h>
%include <Visus/Access.h>
%include <Visus/LogicSamples.h>
%include <Visus/Query.h>
%include <Visus/BlockQuery.h>
%include <Visus/BoxQuery.h>
%include <Visus/PointQuery.h>
%include <Visus/Query.h>

%include <Visus/IdxFile.h>
%include <Visus/DatasetBitmask.h>
%include <Visus/DatasetTimesteps.h>
%include <Visus/Dataset.h>
%include <Visus/ModVisus.h>
%include <Visus/Db.h>

%include <Visus/IdxDataset.h>
%include <Visus/IdxDiskAccess.h>
%include <Visus/IdxMultipleDataset.h>
%include <Visus/VisusConvert.h>




