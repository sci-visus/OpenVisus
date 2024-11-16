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

#include <Visus/DatasetBitmask.h>
#include <Visus/StringUtils.h>

namespace Visus {


//////////////////////////////////////////////////////////////////
DatasetBitmask DatasetBitmask::fromString(String pattern)
{
  if (pattern.empty())
    return DatasetBitmask();
    
  DatasetBitmask ret;
  ret.pattern = pattern;
  for (auto ch : pattern.substr(1))
  {
    int bit= ch -'0';
    if (bit<0) {
      VisusAssert(false);
      return DatasetBitmask();
    }
    ret.pdim =std::max(ret.pdim,bit+1);
    ret.pow2_dims.setPointDim(ret.pdim, 1);
    ret.pow2_dims[bit] <<= 1;
  }
  return ret;
}

//////////////////////////////////////////////////////////////////
DatasetBitmask DatasetBitmask::guess(int first_letter, PointNi dims,bool makeRegularAsSoonAsPossible)
{
  int pdim = dims.getPointDim();

  for (int D=0;D<pdim;D++) 
    dims[D]=(Int64)Utils::getPowerOf2(dims[D]);

  String pattern;

  //example V 00000 01010101
  if (makeRegularAsSoonAsPossible)
  {
    while (dims!=PointNi::one(pdim))
    {
      for (int D=pdim-1;D>=0;D--) 
      {
        if (dims[D]>1) 
        {
          pattern +=('0'+D);
          dims[D]>>=1;
        }
      }
    }

    pattern = StringUtils::reverse(pattern);
  }
  //example V 01010101 00000
  else
  {
    while (dims!=PointNi::one(pdim))
    {
      for (int D=0;D<pdim;D++)
      {
        if (dims[D]>1) 
        {
          pattern +=('0'+D);
          dims[D]>>=1;
        }
      }
    }
  }

  pattern = String(1, first_letter) + pattern;

	auto ret=DatasetBitmask::fromString(pattern);
  VisusReleaseAssert(ret.valid());
  return ret;
}


} //namespace Visus
