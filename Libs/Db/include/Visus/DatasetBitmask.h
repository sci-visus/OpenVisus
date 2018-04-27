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

#ifndef __VISUS_DB_DATASET_BITMASK_H
#define __VISUS_DB_DATASET_BITMASK_H

#include <Visus/Db.h>
#include <Visus/Box.h>

namespace Visus {

#ifndef DatasetBitmaskMaxLen 
#define DatasetBitmaskMaxLen 512
#endif

////////////////////////////////////////////////////////
class VISUS_DB_API DatasetBitmask
{
public:

  VISUS_CLASS(DatasetBitmask)

  //constructor
  DatasetBitmask() {
  }

  //constructor
  //example with only regular: V0101
  //example with regexpr too : V0101{01}*
  DatasetBitmask(String pattern);

  //guess bitmask for a certain number of samples (will store the result in bitmask)
  static DatasetBitmask guess(NdPoint dims, bool makeRegularAsSoonAsPossible = true);

  //invalid
  static inline DatasetBitmask invalid() {
    return DatasetBitmask();
  }

  //empty
  inline bool empty() const {
    return pattern.empty();
  }

  //valid
  inline bool valid() const {
    return !pattern.empty() && pattern[0] == 'V';
  }

  //getMaxResolution (count only regular)
  inline int getMaxResolution() const {
    return max_resolution;
  }

  //hasRegExpr
  inline bool hasRegExpr() const {
    return (int)exploded.size() > (getMaxResolution() + 1);
  }

  //getPointDim (example V0101 returns 2)
  inline int getPointDim() const {
    return pdim;
  }

  //getPow2Dims
  inline const NdPoint& getPow2Dims() const {
    return pow2_dims;
  }

  //getPow2Box
  inline NdBox getPow2Box() const {
    return NdBox(NdPoint(pdim), pow2_dims);
  }

  //clear
  inline void clear() {
    (*this) = DatasetBitmask();
  }

  //operator[]
  inline int operator[](int I) const {
    return exploded[I];
  }

  //operator==
  inline bool operator==(const DatasetBitmask& other) const {
    return pattern == other.pattern;
  }

  //operator==
  inline bool operator!=(const DatasetBitmask& other) const {
    return pattern != other.pattern;
  }

  //upgradeBox
  inline NdBox upgradeBox(NdBox box, int maxh) const
  {
    VisusAssert(maxh >= max_resolution);
    if (maxh == max_resolution) return box;
    for (int M = max_resolution + 1; M <= maxh; M++)
    {
      int bit = (*this)[M];
      box.p1[bit] <<= 1;
      box.p2[bit] <<= 1;
    }
    return box;
  }

  //Address -> Point
  inline NdPoint deinterleave(BigInt z, int max_resolution) const {
    VisusAssert(valid());
    NdPoint p(pdim);
    NdPoint::coord_t one = 1;
    for (NdPoint shift=NdPoint(pdim); z != 0; z >>= 1, max_resolution--)
    {
      int bit = (*this)[max_resolution];
      if ((z & 1) == 1) p[bit] |= one << shift[bit];
      ++shift[bit];
    }
    return p;
  }

  //toString
  inline String toString() const {
    return pattern;
  }

  //operator+
  DatasetBitmask operator+(const DatasetBitmask& other) const {
    return this->valid() && other.valid() ? DatasetBitmask(this->toString() + other.toString().substr(1)) : DatasetBitmask::invalid();
  }

private:

  String           pattern;
  int              max_resolution=0;
  int              pdim = 0;
  NdPoint          pow2_dims;
  std::vector<int> exploded; 

};



} //namespace Visus


#endif //__VISUS_DB_DATASET_BITMASK_H

