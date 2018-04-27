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

#ifndef _VISUS_BIGINT_H__
#define _VISUS_BIGINT_H__

#include <Visus/Kernel.h>
#include <Visus/NumericLimits.h>

#ifndef VISUS_BIGINT_NBITS
#  define VISUS_BIGINT_NBITS 64
#endif

#if VISUS_BIGINT_NBITS==64

  namespace Visus {

    typedef Int64 BigInt;

    //String->BigInt
    inline BigInt cbigint(const String& s) {
      return cint64(s);
    }

    //BigInt->Int64
    inline Int64 cint64(const BigInt& value) {
      return value;
    }

  } //namespace Visus

#else

  #define VISUS_HAS_INT128 1
  #define VISUS_HAS_INT256 1
  #define VISUS_HAS_INT512 1

  #if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) &&     !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
    #define TTMATH_PLATFORM64 1
    #define TTMATH_NOASM      1
  #else
    #define TTMATH_PLATFORM32 1
  #endif

  #include <ttmath/ttmath.h>

  namespace Visus {

    typedef ttmath::Int< TTMATH_BITS(128) >  Int128;
    typedef ttmath::Int< TTMATH_BITS(256) >  Int256;
    typedef ttmath::Int< TTMATH_BITS(512) >  Int512;

    #if VISUS_BIGINT_NBITS==128
      typedef Int128 BigInt;
    #elif VISUS_BIGINT_NBITS==256
      typedef Int256 BigInt;
    #elif VISUS_BIGINT_NBITS==512
      typedef Int512 BigInt;
    #else
      #error "Something wrong with VISUS_BIGINT_NBITS"
    #endif

    //String->BigInt
    inline BigInt cbigint(const String& s) {
      return BigInt(s);
    }

    //BigInt->Int64 (must check that the BigInt fits in Int64)
    inline Int64 cint64(const BigInt& value) {
      Int64 result; 
      bool overflow = value.ToInt(result) ? true : false; 
      VisusAssert(!overflow); 
      return result;
    }

    //BigInt->String
    inline String cstring(const BigInt& v) {
      return v.ToString();
    }

  } //namespace Visus

#endif



#endif //_VISUS_BIGINT_H__
