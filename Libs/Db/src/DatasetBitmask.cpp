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
DatasetBitmask::DatasetBitmask(String pattern) : max_resolution(0),pdim(0)
{
  this->pattern=pattern;

  if (pattern.empty())
    return;

  if (pattern[0]!='V') {
    VisusAssert(false);
    clear();return;
  }
    
  String regular_pattern,regexpr_pattern;
  int A=StringUtils::find(pattern,"{");
  if (A>=0)
  {
    if (!StringUtils::endsWith(pattern,"}*"))
    {
      VisusAssert(false);
      clear();
      return;
    }

    regular_pattern=pattern.substr(0,A); 
    regexpr_pattern=pattern.substr(A+1,pattern.size()-A-3);
  }
  else
  {
    regular_pattern=pattern;
  }

  this->pdim=0;
  VisusAssert(regular_pattern[0]=='V');
  this->exploded.push_back('V');
  for (int I=1;I<(int)regular_pattern.size();I++) 
  {
    int bit=regular_pattern[I]-'0';
    if (bit<0) {VisusAssert(false);clear();return;}
    this->exploded.push_back(bit);
    this->pdim =std::max(this->pdim,bit+1);
  }
  this->max_resolution=(int)exploded.size()-1;


  this->pow2_dims = NdPoint::one(pdim);
  for (int I = 1; I < (int)regular_pattern.size(); I++)
  {
    int bit = regular_pattern[I] - '0';
    this->pow2_dims[bit] <<= 1;
  }

  if (!regexpr_pattern.empty())
  {
    for (int I=0;exploded.size()<DatasetBitmaskMaxLen;I++) 
    {
      int bit=regexpr_pattern[I % regexpr_pattern.size()]-'0';
      if (bit<0) {VisusAssert(false);clear();return;}
      this->exploded.push_back(bit);
      this->pdim =std::max(this->pdim,bit+1);
    }
  }
}



//////////////////////////////////////////////////////////////////
DatasetBitmask DatasetBitmask::guess(NdPoint dims,bool makeRegularAsSoonAsPossible)
{
  int pdim = dims.getPointDim();

  for (int D=0;D<pdim;D++) 
    dims[D]=(NdPoint::coord_t)Utils::getPowerOf2(dims[D]);

  //example V 00000 01010101
  if (makeRegularAsSoonAsPossible)
  {
    String ret;
    while (dims!=NdPoint::one(pdim))
    {
      for (int D=pdim-1;D>=0;D--) 
      {
        if (dims[D]>1) 
        {
          ret+=('0'+D);
          dims[D]>>=1;
        }
      }
    }
    return DatasetBitmask("V" + StringUtils::reverse(ret));
  }
  //example V 01010101 00000
  else
  {
    String ret="V";
    while (dims!=NdPoint::one(pdim))
    {
      for (int D=0;D<pdim;D++)
      {
        if (dims[D]>1) 
        {
          ret+=('0'+D);
          dims[D]>>=1;
        }
      }
    }
    return DatasetBitmask(ret);
  }
}


} //namespace Visus
