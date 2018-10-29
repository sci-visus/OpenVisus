/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#ifndef VISUS_ARRAY_H
#define VISUS_ARRAY_H

#include <Visus/Kernel.h>
#include <Visus/Field.h>
#include <Visus/Position.h>
#include <Visus/HeapMemory.h>
#include <Visus/Url.h>
#include <Visus/Color.h>

#include <set>

namespace Visus {

//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Array : public Object
{
public:

  VISUS_CLASS(Array)

  //dtype
  DType dtype;

  //dimension of the image
  NdPoint dims;

  //Url
  String url;

  //if empty means row major (can be "hzorder" for idx format) 
  String layout;

  //the bounds of the data
  Position bounds;

  //could be that the data needs some clipping
  Position clipping;

  //internal use only
  SharedPtr<HeapMemory> heap=std::make_shared<HeapMemory>();

  //constructor
  Array() {
  }

  //constructor
  Array(NdPoint dims, DType dtype, SharedPtr<HeapMemory> heap_ = SharedPtr<HeapMemory>()) 
    : heap(heap_ ? heap_ : std::make_shared<HeapMemory>())
  {
    if (!this->resize(dims, dtype, __FILE__, __LINE__)) 
      ThrowException("resize of array failed, out of memory");
  }

  //constructor
  Array(NdPoint::coord_t x, DType dtype, SharedPtr<HeapMemory> heap = SharedPtr<HeapMemory>())
    : Array(NdPoint::one(x, 1), dtype, heap) {
  }

  //constructor
  Array(NdPoint::coord_t x, NdPoint::coord_t y, DType dtype, SharedPtr<HeapMemory> heap = SharedPtr<HeapMemory>())
    : Array(NdPoint::one(x, y), dtype, heap) {
  }

  //constructor
  Array(NdPoint::coord_t x, NdPoint::coord_t y, NdPoint::coord_t z, DType dtype, SharedPtr<HeapMemory> heap = SharedPtr<HeapMemory>())
    : Array(NdPoint::one(x, y,z), dtype, heap) {
  }

  //constructor
  Array(const std::vector<Array>& components)
    : Array(components.empty()? NdPoint() : components[0].dims, components.empty() ? DType() : DType((int)components.size(),components[0].dtype))
  {
    for (int I = 0; I < (int)components.size(); I++)
      setComponent(I, components[I]);
  }

  //destructor
  virtual ~Array() {
  }
  
  //valid
  operator bool() const {
    return dtype.valid();
  }

  //getPointDim
  int getPointDim() const {
    return dims.getPointDim();
  }

  //getWidth
  int getWidth() const {
    return (int)dims[0];
  }

  //getHeight
  int getHeight() const {
    return (int)dims[1];
  }

  //getDepth
  int getDepth() const {
    return (int)dims[2];
  }

  //getTotalNumberOfSamples
  Int64 getTotalNumberOfSamples() const {
    return dims.innerProduct();
  }

  //shareProperties
  void shareProperties(Array other)
  {
    this->layout   = other.layout;
    this->bounds   = other.bounds;
    this->clipping = other.clipping;
  }

public:

  //createView
  static Array createView(Array src, NdPoint dims, DType dtype, Int64 c_offset = 0)
  {
    Int64  ret_c_size = dtype.getByteSize(dims);
    Uint8* ret_c_ptr = (Uint8*)(src.c_ptr() + c_offset);
    VisusAssert(ret_c_ptr + ret_c_size <= src.c_ptr() + src.c_size());
    return Array(dims, dtype, SharedPtr<HeapMemory>(HeapMemory::createUnmanaged(ret_c_ptr, ret_c_size)));
  }

  //createView
  static Array createView(Array src, NdPoint::coord_t x, DType dtype, Int64 c_offset = 0){
    return createView(src, NdPoint::one(1).withX(x), dtype, c_offset);
  }

  //createView
  static Array createView(Array src, NdPoint::coord_t x, NdPoint::coord_t y, DType dtype, Int64 c_offset = 0){
    return createView(src, NdPoint::one(2).withX(x).withY(y), dtype, c_offset);
  }

  //createView
  static Array createView(Array src, NdPoint::coord_t x, NdPoint::coord_t y, NdPoint::coord_t z, DType dtype, Int64 c_offset = 0){
    return createView(src, NdPoint::one(3).withX(x).withY(y).withZ(z), dtype, c_offset);
  }

  //fromVector
  template <typename Type>
  static Array fromVector(NdPoint dims, DType dtype, const std::vector<Type>& v)
  {
    VisusAssert(sizeof(Type) == dtype.getByteSize());
    VisusAssert(dims.innerProduct() == v.size());
    Array ret(dims, dtype);
    memcpy(ret.c_ptr(), &v[0], (size_t)ret.c_size());
    return ret;
  }

  //fromVector
  template <typename Type>
  static Array fromVector(DType dtype, const std::vector<Type>& v) {
    auto dims = NdPoint::one(/*pdim*/1);
    dims[0] = (int)v.size();
    return fromVector(dims,dtype,v);
  }

  //isAllZero
  inline bool isAllZero() const{
    return heap->isAllZero();
  }

  //c_capacity
  inline Int64 c_capacity() const{
    return heap->c_capacity();
  }

  //c_size
  inline Int64 c_size() const{
    return heap->c_size();
  }

  //c_ptr
  inline unsigned char* c_ptr(){
    return heap->c_ptr();
  }

  //c_ptr
  template <typename Type>
  inline Type c_ptr() {
    return heap->c_ptr<Type>();
  }

  //c_ptr
#if !SWIG
  inline const unsigned char* c_ptr() const{
    return heap->c_ptr();
  }
#endif

  //shrink
  inline bool shrink(){
    return heap->shrink();
  }

  //fillWithValue
  inline void fillWithValue(int value) {
    memset(c_ptr(), value, (size_t)c_size());
  }

  //resize
  bool resize(NdPoint dims, DType dtype, const char* file, int line)
  {
    if (!heap->resize(dtype.getByteSize(dims), file, line)) return false;
    this->dims = dims;
    this->dtype = dtype;
    return true;
  }

  //resize
  inline bool resize(NdPoint::coord_t x, DType dtype, const char* file, int line){
    return resize(NdPoint::one(1).withX(x), dtype, file, line);
  }

  //resize
  inline bool resize(NdPoint::coord_t x, NdPoint::coord_t y, DType dtype, const char* file, int line){
    return resize(NdPoint::one(2).withX(x).withY(y), dtype, file, line);
  }

  //resize
  inline bool resize(NdPoint::coord_t x, NdPoint::coord_t y, NdPoint::coord_t z, DType dtype, const char* file, int line){
    return resize(NdPoint::one(3).withX(x).withY(y).withZ(z), dtype, file, line);
  }

  //getComponent
  Array getComponent(int C, Aborted aborted = Aborted()) const;

  //setComponent
  bool setComponent(int C, Array src, Aborted aborted = Aborted());

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

};


#if !SWIG

#pragma pack(push, 1)
template <int nbytes>
class Sample { public: Uint8 v[nbytes];};
#pragma pack(pop)

class BitAlignedSample {};
  
//////////////////////////////////////////////////////////////
template <typename Sample>
class GetSamples
{
public:

  //_______________________________________________________
  class Range
  {
  public:

    Sample* ptr;
    Int64   offset;
    Int64   num;

    //constructor
    inline Range(Sample* ptr_,Int64 offset_,Int64 num_) : ptr(ptr_),offset(offset_),num(num_) {
    }

    //operator=
    inline Range& operator=(const Range& other) {
      if (num!=other.num) ThrowException("range with different dimensions");
      memcpy(ptr+offset,other.ptr+other.offset,sizeof(Sample)*num);
      return *this;
    }

    //operator==
    inline bool operator==(const Range& other) {
      if (num!=other.num)  return false;
      return memcmp(ptr+offset,other.ptr+other.offset,sizeof(Sample)*num)==0;
    }
  };

  //constructor
  inline GetSamples() : ptr(nullptr),num(0) {
  }

  //constructor
  inline GetSamples(Array array) 
    : ptr((Sample*)array.c_ptr()),num(array.c_size()/sizeof(Sample))
  {
    VisusAssert(array.dtype.getByteSize()==sizeof(Sample));
    VisusAssert(Utils::isByteAligned(array.dtype.getBitSize()));
  }

  //operator[]
  inline const Sample& operator[](const Int64& index) const {
    VisusAssert(index>=0 && index<num);
    return ptr[index];
  }

  //operator[]
  inline Sample& operator[](const Int64& index) {
    VisusAssert(index>=0 && index<num);
    return ptr[index];
  }

  //range
  inline Range range(const Int64& offset,const Int64& num) {
    VisusAssert((offset+num)<=this->num);
    return Range(ptr,offset,num);
  }

private:
  
  Sample* ptr=nullptr;
  Int64 num=0;

};

//////////////////////////////////////////////////////////////
template <>
class GetSamples<BitAlignedSample>
{
public:

  //_______________________________________________________
  class Range
  {
  public:

    GetSamples& samples;
    Int64 offset;
    Int64 num;

    //constructor
    inline Range(GetSamples& samples_,Int64 offset_,Int64 num_) : samples(samples_),offset(offset_),num(num_) {
    }

    //operator=
    inline Range& operator=(const Range& other) 
    {
      if (num!=other.num || samples.bitsize!=other.samples.bitsize) 
        ThrowException("range not compatible");

      if (samples.is_byte_aligned)
      {
        memcpy(samples.ptr + (size_t)(this->offset*samples.bytesize), other.samples.ptr + (size_t)(other.offset*samples.bytesize), (size_t)(samples.bytesize*num));
      }
      else
      {
        Int64 totbits        = samples.bitsize*num;
        Int64 dst_bit_offset = this->offset*samples.bitsize;
        Int64 src_bit_offset = other.offset*samples.bitsize;
        Int64 b1, b2, done   = 0;

        for (b1 = 0; (!Utils::isByteAligned(dst_bit_offset + b1) || !Utils::isByteAligned(src_bit_offset + b1)) && b1 < totbits; b1++, done++) 
        {
          auto bit_r=Utils::getBit(other.samples.ptr, src_bit_offset + b1);
          Utils::setBit(samples.ptr, dst_bit_offset + b1,bit_r);
        }

        for (b2 = totbits - 1; (!Utils::isByteAligned(dst_bit_offset + b2 + 1) || !Utils::isByteAligned(src_bit_offset + b2 + 1)) && b2 >= b1; b2--, done++) 
        {
          auto bit_r=Utils::getBit(other.samples.ptr, src_bit_offset + b2);
          Utils::setBit(samples.ptr, dst_bit_offset + b2,bit_r);
        }

        if (done != totbits) 
          memcpy(samples.ptr + ((dst_bit_offset + b1) >> 3), other.samples.ptr + ((src_bit_offset + b1) >> 3), (size_t)((1 + (b2 - b1)) >> 3));
      }
      return *this;
    }

    //operator==
    inline bool operator==(const Range& other) {
    
      if (num!=other.num || samples.bitsize!=other.samples.bitsize) 
        return false;

      if (samples.is_byte_aligned)
      {
        return memcmp(samples.ptr + (size_t)(this->offset*samples.bytesize), other.samples.ptr + (size_t)(other.offset*samples.bytesize), (size_t)(samples.bytesize*num))==0;
      }
      else
      {
        Int64 totbits = samples.bitsize*num;
        Int64 dst_bit_offset = this->offset*samples.bitsize;
        Int64 src_bit_offset = other.offset*samples.bitsize;
        Int64 b1, b2, done = 0;

        for (b1 = 0; (!Utils::isByteAligned(dst_bit_offset + b1) || !Utils::isByteAligned(src_bit_offset + b1)) && b1 < totbits; b1++, done++)
        {
          auto bit_w = Utils::getBit(this->samples.ptr, dst_bit_offset + b1);
          auto bit_r = Utils::getBit(other.samples.ptr, src_bit_offset + b1);
          if (bit_w != bit_r) return false;
        }

        for (b2 = totbits - 1; (!Utils::isByteAligned(dst_bit_offset + b2 + 1) || !Utils::isByteAligned(src_bit_offset + b2 + 1)) && b2 >= b1; b2--, done++)
        {
          int bit_w = Utils::getBit(samples.ptr, dst_bit_offset + b2);
          int bit_r = Utils::getBit(other.samples.ptr, src_bit_offset + b2);
          if (bit_w != bit_r) return false;
        }

        if (done != totbits)
          return memcmp(samples.ptr + ((dst_bit_offset + b1) >> 3), other.samples.ptr + ((src_bit_offset + b1) >> 3), (size_t)((1 + (b2 - b1)) >> 3))==0;

        return true;
      }    
    }
  };

  //constructor
  inline GetSamples() {
  }

  //constructor
  inline GetSamples(Array array) 
  {
    this->bitsize         = array.dtype.getBitSize();
    this->is_byte_aligned = Utils::isByteAligned(this->bitsize);
    this->bytesize        = (is_byte_aligned ? this->bitsize : (int)Utils::alignToByte(bitsize)) >> 3;
    this->ptr             = array.c_ptr();
    this->num             = array.getTotalNumberOfSamples();
  }

  //operator[]
  inline Range operator[](const Int64& index) {
    return range(index,1);
  }

  //range
  inline Range range(const Int64& offset,const Int64& num) {
    VisusAssert((offset+num)<=this->num);
    return Range(*this,offset,num);
  }

private:
  
  Uint8*  ptr=nullptr;

  int     bitsize=0;
  bool    is_byte_aligned=false;
  int     bytesize=0;
  Int64   num=0;

};


//////////////////////////////////////////////////////////////
template <typename T>
class GetComponentSamples
{
public:

  T*            ptr = nullptr;
  NdPoint       dims;
  Int64         tot=0;
  int           stride;
  int           C=0;

  //default constructor
  inline GetComponentSamples() {
  }

  //constructor
  inline GetComponentSamples(Array array,int C_) : C(C_)
  {
    VisusAssert(C>= 0 && C<array.dtype.ncomponents());
    VisusAssert(array.dtype.getByteSize()==sizeof(T)*array.dtype.ncomponents());

    this->ptr    = ((T*)array.c_ptr())+C;
    this->dims   = array.dims;
    this->tot    = array.getTotalNumberOfSamples();
    this->stride = array.dtype.ncomponents();
  }

  //operator[]
  inline T& operator[](const Int64& index) {
    VisusAssert(index>=0 && index<tot);
    return ptr[index*stride];
  }

  //operator[]
  inline const T& operator[](const Int64& index) const {
    VisusAssert(index>=0 && index<tot);
    return ptr[index*stride];
  }

};

//////////////////////////////////////////////////////////////
inline void CopySamples(Array& write,Int64 woffset,Array read,Int64 roffset,Int64 tot) {
  GetSamples<BitAlignedSample>(write).range(woffset,tot)=GetSamples<BitAlignedSample>(read).range(roffset,tot);
}

inline bool CompareSamples(Array& write,Int64 woffset,Array read,Int64 roffset,Int64 tot) {
  return GetSamples<BitAlignedSample>(write).range(woffset,tot)==GetSamples<BitAlignedSample>(read).range(roffset,tot);
}

//see http://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/
template<class Operation,typename... Args>
inline bool NeedToCopySamples(Operation& op,DType dtype,Args&&... args)
{
  int bitsize=dtype.getBitSize();
  if (Utils::isByteAligned(bitsize)) 
  {
    switch (int bytesize = bitsize >> 3)
    {
      case    1: return op.template execute<  Sample<   1> >(std::forward<Args>(args)...);
      case    2: return op.template execute<  Sample<   2> >(std::forward<Args>(args)...);
      case    3: return op.template execute<  Sample<   3> >(std::forward<Args>(args)...);
      case    4: return op.template execute<  Sample<   4> >(std::forward<Args>(args)...);
      case    5: return op.template execute<  Sample<   5> >(std::forward<Args>(args)...);
      case    6: return op.template execute<  Sample<   6> >(std::forward<Args>(args)...);
      case    7: return op.template execute<  Sample<   7> >(std::forward<Args>(args)...);
      case    8: return op.template execute<  Sample<   8> >(std::forward<Args>(args)...);
      case    9: return op.template execute<  Sample<   9> >(std::forward<Args>(args)...);
      case   10: return op.template execute<  Sample<  10> >(std::forward<Args>(args)...);
      case   11: return op.template execute<  Sample<  11> >(std::forward<Args>(args)...);
      case   12: return op.template execute<  Sample<  12> >(std::forward<Args>(args)...);
      case   13: return op.template execute<  Sample<  13> >(std::forward<Args>(args)...);
      case   14: return op.template execute<  Sample<  14> >(std::forward<Args>(args)...);
      case   15: return op.template execute<  Sample<  15> >(std::forward<Args>(args)...);
      case   16: return op.template execute<  Sample<  16> >(std::forward<Args>(args)...);
      case   17: return op.template execute<  Sample<  17> >(std::forward<Args>(args)...);
      case   18: return op.template execute<  Sample<  18> >(std::forward<Args>(args)...);
      case   19: return op.template execute<  Sample<  19> >(std::forward<Args>(args)...);
      case   20: return op.template execute<  Sample<  20> >(std::forward<Args>(args)...);
      case   21: return op.template execute<  Sample<  21> >(std::forward<Args>(args)...);
      case   22: return op.template execute<  Sample<  22> >(std::forward<Args>(args)...);
      case   23: return op.template execute<  Sample<  23> >(std::forward<Args>(args)...);
      case   24: return op.template execute<  Sample<  24> >(std::forward<Args>(args)...);
      case   25: return op.template execute<  Sample<  25> >(std::forward<Args>(args)...);
      case   26: return op.template execute<  Sample<  26> >(std::forward<Args>(args)...);
      case   27: return op.template execute<  Sample<  27> >(std::forward<Args>(args)...);
      case   28: return op.template execute<  Sample<  28> >(std::forward<Args>(args)...);
      case   29: return op.template execute<  Sample<  29> >(std::forward<Args>(args)...);
      case   30: return op.template execute<  Sample<  30> >(std::forward<Args>(args)...);
      case   31: return op.template execute<  Sample<  31> >(std::forward<Args>(args)...);
      case   32: return op.template execute<  Sample<  32> >(std::forward<Args>(args)...);
      case   64: return op.template execute<  Sample<  64> >(std::forward<Args>(args)...);
      case  128: return op.template execute<  Sample< 128> >(std::forward<Args>(args)...);
      case  256: return op.template execute<  Sample< 256> >(std::forward<Args>(args)...);
      case  512: return op.template execute<  Sample< 512> >(std::forward<Args>(args)...);
      case 1024: return op.template execute<  Sample<1024> >(std::forward<Args>(args)...);
      default:
        ThrowException("please add a new 'case XX:'");
        break;
    }
  }

  return op.template execute< BitAlignedSample >(std::forward<Args>(args)...);
}

//see http://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/
template<class Operation,typename... Args>
inline bool ExecuteOnCppSamples(Operation& op,DType dtype,Args&&... args)
{
  if (dtype.isVectorOf(DTypes::INT8   )) return op.template execute<Int8   >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::UINT8  )) return op.template execute<Uint8  >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::INT16  )) return op.template execute<Int16  >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::UINT16 )) return op.template execute<Uint16 >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::INT32  )) return op.template execute<Int32  >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::UINT32 )) return op.template execute<Uint32 >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::INT64  )) return op.template execute<Int64  >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::UINT64 )) return op.template execute<Uint64 >(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::FLOAT32)) return op.template execute<Float32>(std::forward<Args>(args)...);
  if (dtype.isVectorOf(DTypes::FLOAT64)) return op.template execute<Float64>(std::forward<Args>(args)...);

  VisusAssert(false);
  return false;
}

#endif //if !SWIG


//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ArrayPlugin 
{
public:

  VISUS_CLASS(ArrayPlugin)

  //constructor
  ArrayPlugin() {
  }

  //destructor
  virtual ~ArrayPlugin() {}

  //handleStatImage
  virtual StringTree handleStatImage(String url) {
    return StringTree();
  }

  //handleLoadImage
  virtual Array handleLoadImage(String url,std::vector<String> args) =0;

  //handleSaveImage
  virtual bool handleSaveImage(String url,Array src,std::vector<String> args) =0;

  //handleLoadImageFromMemory
  virtual Array handleLoadImageFromMemory(SharedPtr<HeapMemory> src,std::vector<String> args) {
    return Array();
  }

};

///////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API DevNullArrayPlugin : public ArrayPlugin
{
public:

  VISUS_NON_COPYABLE_CLASS(DevNullArrayPlugin)

  //constructor
  DevNullArrayPlugin(){
  }

  //destructor
  virtual ~DevNullArrayPlugin(){
  }

  //handleLoadImage
  virtual Array handleLoadImage(String url,std::vector<String> args) override;

  //handleSaveImage
  virtual bool handleSaveImage(String url,Array src,std::vector<String> args) override {
    return Url(url).isFile() && Url(url).getPath()=="/dev/null";
  }

};

///////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API FreeImageArrayPlugin : public ArrayPlugin
{
public:

  VISUS_NON_COPYABLE_CLASS(FreeImageArrayPlugin)

  //constructor
  FreeImageArrayPlugin();

  //destructor
  virtual ~FreeImageArrayPlugin();

  //handleStatImage
  virtual StringTree handleStatImage(String url) override { 
    StringTree info;
    handleLoadImageWithInfo(url,&info,std::vector<String>());
    return info;
  }

  //handleLoadImage
  virtual Array handleLoadImage(String url,std::vector<String> args) override {
    return handleLoadImageWithInfo(url,nullptr,args);
  }

  //handleSaveImage
  virtual bool handleSaveImage(String url,Array src,std::vector<String> args) override;

  //handleLoadImageFromMemory
  virtual Array handleLoadImageFromMemory(SharedPtr<HeapMemory> src,std::vector<String> args) override;

private:

  //handleLoadImage
  Array handleLoadImageWithInfo(String url,StringTree* info,std::vector<String> args);

};

///////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RawArrayPlugin : public ArrayPlugin
{
public:

  VISUS_NON_COPYABLE_CLASS(RawArrayPlugin)

  std::set<String> extensions=std::set<String>{".raw",".bin",".brick",".dat"};
  
  //constructor
  RawArrayPlugin(){
  }

  //destructor
  virtual ~RawArrayPlugin(){
  }

  //handleStatImage
  virtual StringTree handleStatImage(String url) override;

  //handleLoadImage
  virtual Array handleLoadImage(String url,std::vector<String> args) override;

  //handleSaveImage
  virtual bool handleSaveImage(String url,Array src,std::vector<String> args) override;
};

////////////////////////////////////////////////////
class VISUS_KERNEL_API ArrayPlugins
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(ArrayPlugins)

  std::vector< SharedPtr<ArrayPlugin> > values;

  //destructor
  ~ArrayPlugins() {}

private:
  
  //singleton class 
  ArrayPlugins() {
    this->values.push_back(std::make_shared<DevNullArrayPlugin>());
    this->values.push_back(std::make_shared<RawArrayPlugin>());
    this->values.push_back(std::make_shared<FreeImageArrayPlugin>());
  }

};

/////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ComputeRange
{
public:

  VISUS_CLASS(ComputeRange)

  //how to map input data to [0,1]
  enum Mode
  {
    UseArrayRange,
    PerComponentRange,
    ComputeOverallRange,
    UseCustomRange
  };

  Mode mode;

  //used IIF mode==UseCustom
  Range custom_range; 

  //constructor
  ComputeRange(Mode mode_=UseArrayRange) : mode(mode_){
  }

  //createCustom
  static ComputeRange createCustom(Range range) {
    ComputeRange ret(UseCustomRange);
    ret.custom_range = range;
    return ret;
  }

  //createCustom
  static ComputeRange createCustom(double A, double B) {
    return createCustom(Range(A, B, 0));
  }

  //isCustom
  bool isCustom() const {
    return mode == UseCustomRange;
  }

  //doCompute
  Range doCompute(Array src, int ncomponent,Aborted aborted = Aborted());

};

///////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ArrayUtils
{
public:

  //loadImage
  static Array loadImage(String url,std::vector<String> args=std::vector<String>());

  //loadImageFromMemory
  static Array loadImageFromMemory(String url,SharedPtr<HeapMemory> heap,std::vector<String> args=std::vector<String>());

  //statImage
  static StringTree statImage(String url);

  //saveImage
  static bool saveImage(String url,Array src,std::vector<String> args=std::vector<String>());

  //saveImageUINT8
  static bool saveImageUINT8(String url,Array src,std::vector<String> args=std::vector<String>()) {
    return saveImage(url,smartCast(src,DType(src.dtype.ncomponents(),DTypes::UINT8)),args);
  }

  //encodeArray
  static SharedPtr<HeapMemory> encodeArray(String compression,Array value);

  //decodeArray
  static Array decodeArray(String compression, NdPoint dims,DType dtype,SharedPtr<HeapMemory> encoded);

  //decodeArray
  static Array decodeArray(StringMap metadata, SharedPtr<HeapMemory> encoded);

public:

   //deepCopy
  static bool deepCopy(Array& dst, Array src)
  {
    SharedPtr<HeapMemory> heap(src.heap->clone());
    if (!heap) return false;
    dst = src;
    dst.heap = heap;
    return true;
  }
 
  //computeRange
  static Range computeRange(Array src, int C, Aborted aborted = Aborted()) {
    return ComputeRange(ComputeRange::PerComponentRange).doCompute(src,C,aborted);
  }

  //interleave
  static Array interleave(std::vector<Array> v, Aborted aborted = Aborted());

  //insert
  static bool insert(
    Array& wbuffer, NdPoint wfrom, NdPoint wto, NdPoint wstep,
    Array  rbuffer, NdPoint rfrom, NdPoint rto, NdPoint rstep, Aborted aborted = Aborted());

  // interpolate
  static bool interpolate(Array& dst, NdPoint from, NdPoint to, NdPoint step, Aborted aborted = Aborted());

  //paste src image into dst image
  static bool paste(Array& dst,NdBox Dbox, Array src, NdBox Sbox, Aborted aborted = Aborted());

  //paste
  static bool paste(Array& dst,NdPoint offset, Array src, Aborted aborted = Aborted()) {
    int pdim = src.getPointDim();
    return paste(dst,NdBox(offset,offset + src.dims), src, NdBox(NdPoint(pdim),src.dims));
  }

  //smartCast
  static Array smartCast(Array src, DType dtype, Aborted aborted = Aborted());

  //crop
  static Array crop(Array src, NdBox box, Aborted aborted = Aborted());

  //mirror
  static Array mirror(Array src, int axis, Aborted aborted = Aborted());

  //downsample (remove odd samples)
  static Array downSample(Array src, int bit, Aborted aborted = Aborted());

  //upsample (replicate samples)
  static Array upSample(Array src, int bit, Aborted aborted = Aborted());

  //splitAndGetFirst
  static Array splitAndGetFirst(Array src, int axis, Aborted aborted = Aborted());

  //splitAndGetSecond
  static Array splitAndGetSecond(Array src, int axis, Aborted aborted = Aborted());

  //cast
  static Array cast(Array src, DType dtype, Aborted aborted = Aborted());

  //sqrt
  static Array sqrt(Array src, Aborted aborted = Aborted());

  //module2
  static Array module2(Array input, Aborted aborted) 
  {
    Array ret(input.dims, DTypes::FLOAT64);
    ret.fillWithValue(0);
    for (int I = 0; I < input.dtype.ncomponents(); I++)
    {
      auto casted=cast(input.getComponent(I), DTypes::FLOAT64);
      ret = ArrayUtils::add(ret, ArrayUtils::mul(casted, casted, aborted), aborted);
    }
    return ret;
  }

  //module
  static Array module(Array input, Aborted aborted) {
    return sqrt(module2(input, aborted), aborted);
  }

  //add
  static Array add(Array a, double b, Aborted aborted = Aborted());

  //sub
  static Array sub(double a, Array b,Aborted aborted = Aborted());

  //sub
  static Array sub(Array a, double b, Aborted aborted = Aborted());

  //mul
  static Array mul(Array src, double coeff, Aborted aborted = Aborted());

  //div
  static Array div(Array src, double coeff, Aborted aborted = Aborted()){
    return mul(src, 1.0 / coeff, aborted);
  }

  //div
  static Array div(double coeff, Array src, Aborted aborted = Aborted());

  //resample
  static Array resample(NdPoint target_dims, const Array rbuffer, Aborted aborted = Aborted());

public:

  //convolve (see http://en.wikipedia.org/wiki/Kernel_(image_processing))
  //very good explanation at http://www.cs.cornell.edu/courses/CS1114/2013sp/sections/S06_convolution.pdf
  //see http://www.johnloomis.org/ece563/notes/filter/conv/convolution.html

  //convolve
  static Array convolve(Array src, Array kernel, Aborted aborted = Aborted());

  //medianHybrid
  static Array medianHybrid(Array src, Array krn_size, Aborted aborted = Aborted());

  //median
  static Array median(Array src, const Array krn_size, int percent, Aborted aborted = Aborted());

public:

  //executeOperation
  enum Operation
  {
    InvalidOperation,

    AddOperation,
    SubOperation,
    MulOperation,
    DivOperation,
    MinOperation,
    MaxOperation,

    AverageOperation,
    StandardDeviationOperation,
    MedianOperation
  };

  static Array executeOperation(Operation op, std::vector<Array> args, Aborted aborted = Aborted());

  static Array executeOperation(Operation op, Array a, Array b, Aborted aborted = Aborted()){
    return executeOperation(op,{a,b}, aborted);
  }

  //add
  static Array add(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(AddOperation, args, aborted);
  }

  //add
  static Array add(Array a, Array b, Aborted aborted = Aborted()) {
    return add({ a,b }, aborted);
  }

  //sub
  static Array sub(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(SubOperation, args, aborted);
  }

  //sub
  static Array sub(Array a, Array b, Aborted aborted = Aborted()) {
    return sub({ a,b }, aborted);
  }

  //mul
  static Array mul(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(MulOperation, args, aborted);
  }

  //mul
  static Array mul(Array a, Array b, Aborted aborted = Aborted()) {
    return mul({ a,b }, aborted);
  }

  //div
  static Array div(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(DivOperation, args, aborted);
  }

  //div
  static Array div(Array a, Array b, Aborted aborted = Aborted()) {
    return div({ a,b }, aborted);
  }

  //min
  static Array min(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(MinOperation, args, aborted);
  }

  //min
  static Array min(Array a, Array b, Aborted aborted = Aborted()) {
    return min({ a,b }, aborted);
  }

  //max
  static Array max(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(MaxOperation, args, aborted);
  }

  //max
  static Array max(Array a, Array b, Aborted aborted = Aborted()) {
    return max({ a,b }, aborted);
  }

  //average
  static Array average(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(AverageOperation, args, aborted);
  }

  //average
  static Array average(Array a, Array b, Aborted aborted = Aborted()) {
    return average({ a,b }, aborted);
  }

  //standardDeviation
  static Array standardDeviation(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(StandardDeviationOperation, args, aborted);
  }

  //standardDeviation
  static Array standardDeviation(Array a, Array b, Aborted aborted = Aborted()) {
    return standardDeviation({ a,b }, aborted);
  }

  //median
  static Array median(std::vector<Array> args, Aborted aborted = Aborted()) {
    return executeOperation(MedianOperation, args, aborted);
  }

  //median
  static Array median(Array a, Array b, Aborted aborted = Aborted()) {
    return median({ a,b }, aborted);
  }

public:

  //see http://pippin.gimp.org/image_processing/chap_point.html

  //threshold (level in 0,1 range)
  static Array threshold(Array src,double level, Aborted aborted = Aborted());

  // brightnessContrast
  static Array brightnessContrast(Array src, double brightness = 0.0, double contrast = 1.0, Aborted aborted = Aborted());

  //invert
  static Array invert(Array src, Aborted aborted = Aborted());

  //levels
  static Array levels(Array src, double gamma, double in_min, double in_max, double out_min, double out_max, Aborted aborted = Aborted());

  //hueSaturationBrightness (HSB)
  static Array hueSaturationBrightness(Array src, double hue, double saturation, double brightness, Aborted aborted = Aborted());

public:

  //warpPerspective
  static bool warpPerspective(Array& dst, Array& dst_alpha, Matrix T,Array src,Array src_alpha, Aborted aborted);

  //setBufferColor
  static void setBufferColor(Array& buffer, const Array& alpha, Color color);

  //createTransformedAlpha
  static Array createTransformedAlpha(NdBox bounds, Matrix T, NdPoint dims, Aborted aborted);

private:

  ArrayUtils() = delete;

};


////////////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API BlendBuffers
{
public:

  VISUS_PIMPL_CLASS(BlendBuffers)
  
  enum Type
  {
    GenericBlend,
    NoBlend,
    AverageBlend,
    VororoiBlend
  };

  Array result;

  //constructor
  BlendBuffers(Type type, Aborted aborted_);

  //destructor
  ~BlendBuffers();

  //addArg
  void addArg(Array buffer, Array alpha, Matrix up_pixel_to_logic= Matrix(), Point3d logic_centroid=Point3d());

};


} //namespace Visus

#endif //VISUS_ARRAY_H



