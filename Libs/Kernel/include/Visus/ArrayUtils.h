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

#ifndef VISUS_ARRAY_UTILS_H
#define VISUS_ARRAY_UTILS_H

#include <Visus/Array.h>

namespace Visus {




///////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ArrayUtils
{
public:


  //loadImage
  static Array loadImage(String url, std::vector<String> args = std::vector<String>());

  //loadImageFromMemory
  static Array loadImageFromMemory(String url, SharedPtr<HeapMemory> heap, std::vector<String> args = std::vector<String>());

  //statImage
  static StringTree statImage(String url);

  //saveImage
  static bool saveImage(String url, Array src, std::vector<String> args = std::vector<String>());

  //saveImageUINT8
  static bool saveImageUINT8(String url, Array src, std::vector<String> args = std::vector<String>()) {
    return saveImage(url, smartCast(src, DType(src.dtype.ncomponents(), DTypes::UINT8)), args);
  }

  //encodeArray
  static SharedPtr<HeapMemory> encodeArray(String compression, Array value);

  //decodeArray
  static Array decodeArray(String compression, NdPoint dims, DType dtype, SharedPtr<HeapMemory> encoded);

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
    return ComputeRange(ComputeRange::PerComponentRange).doCompute(src, C, aborted);
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
  static bool paste(Array& dst, NdBox Dbox, Array src, NdBox Sbox, Aborted aborted = Aborted());

  //paste
  static bool paste(Array& dst, NdPoint offset, Array src, Aborted aborted = Aborted()) {
    int pdim = src.getPointDim();
    return paste(dst, NdBox(offset, offset + src.dims), src, NdBox(NdPoint(pdim), src.dims));
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

  //selectComponents (equivalent to numpy numpy[A:B])
  static Array selectComponents(Array src, std::vector<int> v, Aborted aborted = Aborted());

  //sqrt
  static Array sqrt(Array src, Aborted aborted = Aborted());

  //module2
  static Array module2(Array input, Aborted aborted)
  {
    Array ret(input.dims, DTypes::FLOAT64);
    ret.fillWithValue(0);
    for (int I = 0; I < input.dtype.ncomponents(); I++)
    {
      auto casted = cast(input.getComponent(I), DTypes::FLOAT64);
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
  static Array sub(double a, Array b, Aborted aborted = Aborted());

  //sub
  static Array sub(Array a, double b, Aborted aborted = Aborted());

  //mul
  static Array mul(Array src, double coeff, Aborted aborted = Aborted());

  //div
  static Array div(Array src, double coeff, Aborted aborted = Aborted()) {
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

  static Array executeOperation(Operation op, Array a, Array b, Aborted aborted = Aborted()) {
    return executeOperation(op, { a,b }, aborted);
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
  static Array threshold(Array src, double level, Aborted aborted = Aborted());

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
  static bool warpPerspective(Array& dst, Array& dst_alpha, Matrix T, Array src, Array src_alpha, Aborted aborted);

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
  void addArg(Array buffer, Array alpha, Matrix up_pixel_to_logic = Matrix(), Point3d logic_centroid = Point3d());

};

} //namespace Visus

#endif //VISUS_ARRAY_UTILS_H

