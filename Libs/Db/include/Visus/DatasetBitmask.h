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
  static DatasetBitmask fromString(String pattern);

  //guess bitmask for a certain number of samples (will store the result in bitmask)
  //first letter == 'V' | 'F'
  //scrgiorgio 202311005 changing `makeRegularAsSoonAsPossible` from true to false
  //           from experience it seeems it is always better to have regular pattern on the left near the V
  static DatasetBitmask guess(int first_letter, PointNi dims, bool makeRegularAsSoonAsPossible = false);

  //invalid
  static DatasetBitmask invalid() {
    return DatasetBitmask();
  }

  //empty
  bool empty() const {
    return pattern.empty();
  }

  //valid
  bool valid() const {
    return !pattern.empty() && (pattern[0] == 'V' || pattern[0] == 'F');
  }

  //getMaxResolution 
  int getMaxResolution() const {
    return (int)pattern.size()-1;
  }

  //getPointDim
  int getPointDim() const {
    return pdim;
  }

  //getPow2Dims
  const PointNi& getPow2Dims() const {
    return pow2_dims;
  }

  //getPow2Box
  BoxNi getPow2Box() const {
    return BoxNi(PointNi(pdim), pow2_dims);
  }

  //operator[]
  int operator[](int I) const {
    return (int)(I? pattern[I]-'0' : pattern[0]);
  }

  //operator==
  bool operator==(const DatasetBitmask& other) const {
    return pattern == other.pattern;
  }

  //operator==
  bool operator!=(const DatasetBitmask& other) const {
    return pattern != other.pattern;
  }

  //Address -> PointNd
  PointNi deinterleave(BigInt z, int max_resolution) const {
    VisusAssert(valid());
    PointNi p(pdim);
    Int64 one = 1;
    for (PointNi shift=PointNi(pdim); z != 0; z >>= 1, max_resolution--)
    {
      int bit = (*this)[max_resolution];
      if ((z & 1) == 1) p[bit] |= one << shift[bit];
      ++shift[bit];
    }
    return p;
  }

  //toString
  const String& toString() const {
    return pattern;
  }

  //operator+
  static DatasetBitmask add(const DatasetBitmask& a,const DatasetBitmask& b)  {
    return a.valid() && b.valid() ? DatasetBitmask::fromString(a.pattern + b.pattern.substr(1)) : DatasetBitmask::invalid();
  }

private:

  String           pattern;
  int              pdim = 0;
  PointNi          pow2_dims;

};



} //namespace Visus


#endif //__VISUS_DB_DATASET_BITMASK_H

