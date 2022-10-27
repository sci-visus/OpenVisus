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

#ifndef VISUS_ZFP_ENCODER_H
#define VISUS_ZFP_ENCODER_H

#include <Visus/Kernel.h>
#include <Visus/Encoder.h>

#include <zfp.h>
#include <cstdint>

namespace Visus {


/* compress or decompress array */
//see zfp/examples/simple.c 

//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ZfpEncoder : public Encoder
{
  
public:

  VISUS_CLASS(ZfpEncoder)

  //up to 64..
  String encode_specs;
  String decode_specs;

  //constructor
  ZfpEncoder(String specs) 
  { 
    if (specs == "zfp")
      specs = "zfp-reversible-64"; //reversible for encoding, 64 for decoding precision

    //see https://zfp.readthedocs.io/en/release1.0.0/modes.html

    //example:
    //  zfp-<encode_spec>-<decode_spec> 
    //  specs:=(reversible) | (precision=<num::int>) | (accuracy=<value::double>)
    auto v = StringUtils::split("-");
    VisusReleaseAssert(v.size() == 3);
    VisusReleaseAssert(v[0] == "zfp");
    this->encode_specs = StringUtils::trim(v[1]);
    this->decode_specs = StringUtils::trim(v[2]);
  }

  //destructor
  virtual ~ZfpEncoder() {
  }

  //isLossy
  virtual bool isLossy() const override {
    return true; //? lossy in decompression, but when I compress it should be not-lossy?
  }

  //getZfpType
  zfp_type getZfpType(DType dtype)
  {
    if (dtype==DTypes::FLOAT64)
      return (zfp_type)zfp_type_double;

    if (dtype == DTypes::FLOAT32)
      return (zfp_type)zfp_type_float;

    if (dtype == DTypes::INT64)
      return (zfp_type)zfp_type_int64;

    if (dtype == DTypes::INT32)
      return (zfp_type)zfp_type_int32;

    VisusReleaseAssert(false);
  }

  //setStreamOptions
  static zfp_stream* createStream(String specs)
  {
    zfp_stream* zfp=zfp_stream_open(NULL);;

    if (specs.empty() || specs == "reversible")
    {
      zfp_stream_set_reversible(zfp);
      return zfp;
    }

    if (StringUtils::startsWith(specs, "precision="))
    {
      int precision = cint(specs.substr(String("precision=").size()));
      zfp_stream_set_precision(zfp, precision);
      return zfp;
    }

    if (StringUtils::startsWith(specs, "accuracy="))
    {
      double accuracy = cdouble(specs.substr(String("accuracy=").size()));
      zfp_stream_set_accuracy(zfp, accuracy);
      return zfp;
    }

    VisusReleaseAssert(false); //todo other cases
    return zfp;
  }

  //encode
  virtual SharedPtr<HeapMemory> encode(PointNi dims, DType dtype, SharedPtr<HeapMemory> decoded) override
  {
    if (!decoded)
      return SharedPtr<HeapMemory>();

    auto pdim = dims.getPointDim();
    VisusReleaseAssert(pdim == 3);
    VisusReleaseAssert(dtype.ncomponents()==1);

    zfp_field* field = zfp_field_3d(decoded->c_ptr(), getZfpType(dtype), dims[0], dims[1], dims[2]);

    zfp_stream* zfp = createStream(encode_specs);

    size_t encoded_size = zfp_stream_maximum_size(zfp, field);
    auto encoded = std::make_shared<HeapMemory>();
    if (!encoded->resize(encoded_size, __FILE__, __LINE__))
      return SharedPtr<HeapMemory>();

    bitstream* stream = stream_open(encoded->c_ptr(), encoded->c_size());
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    size_t zfpsize = zfp_compress(zfp, field);
    VisusReleaseAssert(zfpsize);
    encoded->resize(zfpsize, __FILE__, __LINE__);

    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    return encoded;
  }

  //decode
  virtual SharedPtr<HeapMemory> decode(PointNi dims, DType dtype, SharedPtr<HeapMemory> encoded) override
  {
    static int counter = 0;

    if (!encoded)
      return SharedPtr<HeapMemory>();
    
    auto pdim = dims.getPointDim();
    VisusReleaseAssert(pdim == 3);
    VisusReleaseAssert(dtype.ncomponents() == 1);

    auto decoded = std::make_shared<HeapMemory>();
    if (!decoded->resize(dtype.getByteSize(dims), __FILE__, __LINE__))
      return SharedPtr<HeapMemory>();

    zfp_field* field = zfp_field_3d(decoded->c_ptr(), getZfpType(dtype), dims[0], dims[1], dims[2]);
    zfp_stream* zfp = createStream(decode_specs);

    bitstream* stream = stream_open(encoded->c_ptr(), encoded->c_size());
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    size_t result = zfp_decompress(zfp, field);

    //result is probably how much of the encoded stream I've used, i think it can use the compressed stream even partially
    VisusReleaseAssert(result!=0);

    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    return decoded;
  }

  //test
  static void test()
  {
    auto dims = PointNi(16, 32, 64);
    auto dtype = DTypes::FLOAT64;
    auto decoded = std::make_shared<HeapMemory>();
    decoded->resize(dtype.getByteSize(dims), __FILE__, __LINE__);
    double* ptr = (double*)decoded->c_ptr();
    for (int k = 0; k < dims[2]; k++) {
      for (int j = 0; j < dims[1]; j++) {
        for (int i = 0; i < dims[0]; i++)
        {
          double x = 2.0 * i / dims[0];
          double y = 2.0 * j / dims[1];
          double z = 2.0 * k / dims[2];
          ptr[i + dims[0] * (j + dims[1] * k)] = exp(-(x * x + y * y + z * z));
        }
      }
    }

    auto encoder = Encoders::getSingleton()->createEncoder("zfp");
    auto encoded = encoder->encode(dims, dtype, decoded);
    auto check = encoder->decode(dims, dtype, encoded);
    VisusReleaseAssert(HeapMemory::equals(decoded, check));
  }

};

} //namespace Visus

#endif //VISUS_ZFP_ENCODER_H

