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

#ifndef __VISUS_IDX_HZORDER_H
#define __VISUS_IDX_HZORDER_H

#include <Visus/Idx.h>
#include <Visus/DatasetBitmask.h>

namespace Visus {



/* ------------------------------------------------------- 
LOW-LEVEL mapping functions. Remember this schema

      ZADDRESS           HZADDRESS
       V010101{01}*      
   H=0  000000                   0
   H=1  100000                   1
   H=2  x10000                  1x
   H=3  xx1000                 1xx
   H=4  xxx100                1xxx
   H=5  xxxx10               1xxxx
   H=6  xxxxx1              1xxxxx
   H=7  xxxxxx{1}          1xxxxxx
   H=8  xxxxxx{x1}        1xxxxxxx  (can explode regex)
* ------------------------------------------------------- */

class VISUS_IDX_API HzOrder
{
public:

  VISUS_CLASS(HzOrder)

  //default constructor
  HzOrder()
  {}

  //constructor
  inline HzOrder(const DatasetBitmask& bitmask_,int maxh_) : bitmask(bitmask_),maxh(maxh_),pdim(bitmask_.getPointDim())
  {}

  //getBitmask
  const DatasetBitmask& getBitmask() const
  {return bitmask;}

  //getMaxResolution
  inline int getMaxResolution() const
  {return maxh;}

  //Point -> Zaddress
  /* EXAMPLE
                             01234
                             -----
                             V0101 maxh=4
                             -----
                              8421
                             -----
    interleave(NdPoint(0 0))= 0000= 0
    interleave(NdPoint(1 0))= 0010= 2
    interleave(NdPoint(2 0))= 1000= 8
    interleave(NdPoint(3 0))= 1010=10
    interleave(NdPoint(0 1))= 0001= 1
    interleave(NdPoint(1 1))= 0011= 3
    interleave(NdPoint(2 1))= 1001= 9
    interleave(NdPoint(3 1))= 1011=11
    interleave(NdPoint(0 2))= 0100= 4
    interleave(NdPoint(1 2))= 0110= 6
    interleave(NdPoint(2 2))= 1100=12
    interleave(NdPoint(3 2))= 1110=14
    interleave(NdPoint(0 3))= 0101= 5
    interleave(NdPoint(1 3))= 0111= 7
    interleave(NdPoint(2 3))= 1101=13
    interleave(NdPoint(3 3))= 1111=15
  */

  inline BigInt interleave(NdPoint p) const
  {
    VisusAssert(bitmask.valid());
    int maxh=this->maxh;
    BigInt z=0;
    NdPoint zero(pdim);
    for (int shift=0;p!=zero;shift++,maxh--)
    {
      int bit=bitmask[maxh];
      z |= ((BigInt)p[bit] & 1) << shift;
      p[bit] >>= 1;
    }
    return z;
  }

  //Zaddress -> Point
  inline NdPoint deinterleave(BigInt z) const
  {return bitmask.deinterleave(z,this->maxh);}

  //getZStartAddress  (Replace the ..xxx.. into 0)
  /*  
                              01234
                              -----
                              V0101 maxh=4
                              -----
                               8421
                              -----
    V0000 getZStartAddress(0)= 0000= 0
    V1000 getZStartAddress(1)= 1000= 8
    Vx100 getZStartAddress(2)= 0100= 4
    Vxx10 getZStartAddress(3)= 0010= 2
    Vxxx1 getZStartAddress(4)= 0001= 1
  */
  inline BigInt getZStartAddress(int H) const
  {
    VisusAssert(H>=0 && H<=maxh);
    return H? ((BigInt)1)<<(maxh-H) : 0;
  }

  //getZEndAddress (Replace the ..xxx.. into 1)
  /* 
                            01234
                            -----
                            V0101 maxh=4
                            -----
                             8421
                            -----
    V0000 getZEndAddress(0)= 0000= 0
    V1000 getZEndAddress(1)= 1000= 8
    Vx100 getZEndAddress(2)= 1100=12
    Vxx10 getZEndAddress(3)= 1110=14
    Vxxx1 getZEndAddress(4)= 1111=15
  */
  inline BigInt getZEndAddress(int H) const
  {
    VisusAssert(H>=0 && H<=maxh);
    return H? ((((BigInt)1)<<maxh)-getZStartAddress(H)) : (0);
  }

  //Zaddress -> HzAddress (see table above)
  inline BigInt zAddressToHzAddress(BigInt z) const
  {
    BigInt last_bitmask=((BigInt)1)<<maxh; //a "1" enter in the left
    z |= last_bitmask;
    BigInt one=1;
    while ((one & z)==0) z>>=1; //until a "1" exit
    z >>= 1;
    return z;
  }

  //HzAddress -> Zaddress (see table above)
  inline BigInt hzAddressToZAddress(BigInt hz) const
  {
    BigInt last_bitmask=((BigInt)1)<<maxh;
    hz <<= 1;
    hz  |= 1;
    while ((last_bitmask & hz)==0) hz<<=1;
    hz &= last_bitmask - 1;
    return hz;
  }

  //Point -> HzAddress
  inline BigInt getAddress(const NdPoint& p) const
  {return zAddressToHzAddress(interleave(p));}

  //HzAddress -> Point
  inline NdPoint getPoint(const BigInt& hz) const
  {return deinterleave(hzAddressToZAddress(hz));}


  //getLevelDelta (count the right 0 and the blocking 1)
  /*
    01234 
    -----
    v0101 maxh=4
    -----
    V0000  getLevelDelta(0)=(4 4) V
    V1000  getLevelDelta(1)=(4 4) 0
    Vx100  getLevelDelta(2)=(2 4) 1
    Vxx10  getLevelDelta(3)=(2 2) 0
    Vxxx1  getLevelDelta(4)=(1 2) 1
  */
  inline NdPoint getLevelDelta(int H) const
  {
    VisusAssert(H>=0 && H<=maxh);
    NdPoint p=NdPoint::one(pdim);
    if (!H) H=1;
    for (int K=maxh;K>=H;K--)
      p[bitmask[K]]<<=1;
    return p;
  }

  //getLevelP1 (set the ...xx.. to 0)
  /*
    01234 maxh=4
    -----
    v0101
    -----
    V0000  getLevelP1(0)=V0000=(0,0)
    V1000  getLevelP1(1)=V1000=(2,0)
    Vx100  getLevelP1(2)=V0100=(0,2)
    Vxx10  getLevelP1(3)=V0010=(1,0)
    Vxxx1  getLevelP1(4)=V0001=(0,1)
  */
  inline NdPoint getLevelP1(int H) const
  {
    VisusAssert(H>=0 && H<=maxh);
    if (!H) return NdPoint(pdim);
    return deinterleave(getZStartAddress(H));
  }

  //getLevelP2Included (set the ...xx.. to 1)
  /*
    01234 maxh=4
    -----
    v0101
    -----
    V0000  getLevelP2Included(0)=V0000=(0,0)
    V1000  getLevelP2Included(1)=V1000=(2,0)
    Vx100  getLevelP2Included(2)=V1100=(2,2)
    Vxx10  getLevelP2Included(3)=V1110=(3,2)
    Vxxx1  getLevelP2Included(4)=V1111=(3,3)
  */
  inline NdPoint getLevelP2Included(int H) const
  {
    VisusAssert(H>=0 && H<=maxh);
    if (!H) return NdPoint(pdim);
    return deinterleave(getZEndAddress(H));
  }

  //the right-most "1" set (the bit that will become the V in the right shift in a bitmask such as V010101...)
  static inline int getAddressResolution(const DatasetBitmask& bitmask,BigInt hz)
  {
    int ret=0;
    while (hz!=0) {ret++;hz>>=1;}
    return ret;
  }

  //getAddressRangeNumberOfSamples (i.e. samples for each axis)
  static inline NdPoint getAddressRangeNumberOfSamples(const DatasetBitmask& bitmask,BigInt hzfrom,BigInt hzto)
  {
    int bitsperblock=Utils::getLog2(cint64(hzto-hzfrom));
    NdPoint nsamples=NdPoint::one(bitmask.getPointDim());

    //remember! block 1 has the same number of samples of block 0 and block 1 has hzResolution()=bitsperblock+1
    int H=(hzfrom!=0)? getAddressResolution(bitmask,hzfrom) : (bitsperblock+1); 
  
    //all the bits on the left of the H (example bitsperblock=3 H=7  V010.101.0 nsamples=2*4)
    for (int N=H-bitsperblock;N<H;N++)
    {
      VisusAssert(N>=1);
      nsamples[bitmask[N]]<<=1;
    }

    VisusAssert(nsamples.innerProduct()==cint64(hzto-hzfrom));
    return nsamples;
  }

private:

  DatasetBitmask bitmask;
  int maxh = 0;
  int pdim=0;

};


} //namespace Visus

#endif //__VISUS_IDX_HZORDER_H
