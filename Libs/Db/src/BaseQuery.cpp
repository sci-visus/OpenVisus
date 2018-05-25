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

#include <Visus/BaseQuery.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////////////////////////
bool BaseQuery::mergeSamples(LogicBox Wsamples,Array& Wbuffer,LogicBox Rsamples,Array Rbuffer,int merge_mode,Aborted aborted)
{
  if (!Wsamples.valid() || !Rsamples.valid())
    return false;

  //cannot find intersection (i.e. no sample to merge)
  NdBox box= Wsamples.getIntersection(Rsamples);
  if (!box.isFullDim())
    return false;

  /*
  Example of the problem to solve:

    Wsamples:=-2 + kw*6         -2,          4,          10,            *16*,            22,            28,            34,            40,            *46*,            52,            58,  ...)
    Rsamples:=-4 + kr*5    -4,         1,        6,         11,         *16*,          21,       25,            ,31         36,          41,         *46*,          51,         56,       ...)

    give kl,kw,kr independent integers all >=0

    leastCommonMultiple(2,6,5)= 2*3*5 =30 

    First "common" value (i.e. minimum value satisfying all 3 conditions) is 16
  */

  int pdim = Rbuffer.getPointDim();
  NdPoint delta(pdim);
  for (int D=0;D<pdim;D++)
  {
    NdPoint::coord_t lcm=Utils::leastCommonMultiple(Rsamples.delta[D],Wsamples.delta[D]);

    NdPoint::coord_t P1=box.p1[D];
    NdPoint::coord_t P2=box.p2[D];

    while (!Utils::isAligned(P1,Wsamples.p1[D],Wsamples.delta[D]) ||
           !Utils::isAligned(P1,Rsamples.p1[D],Rsamples.delta[D]))
    {
      //NOTE: if the value is already aligned, alignRight does nothing
      P1=Utils::alignRight(P1,Wsamples.p1[D],Wsamples.delta[D]);
      P1=Utils::alignRight(P1,Rsamples.p1[D],Rsamples.delta[D]);

      //cannot find any alignment, going beyond the valid range
      if (P1>=P2)
        return false;

      //continue in the search IIF it's not useless
      if ((P1-box.p1[D])>=lcm)
      {
        //should be acceptable to be here, it just means that there are no samples to merge... 
        //but since 99% of the time Visus has pow-2 alignment it is high unlikely right now... adding the VisusAssert just for now
        VisusAssert(false);
        return false;
      }
    }

    delta[D]=lcm;
    P2=Utils::alignRight(P2,P1,delta[D]);
    box.p1[D]=P1;
    box.p2[D]=P2;
  }

  VisusAssert(box.isFullDim());

  VisusAssert(Wbuffer.dims==Wsamples.nsamples);
  VisusAssert(Rbuffer.dims==Rsamples.nsamples);
  VisusAssert(Wbuffer.dtype==Rbuffer.dtype);

  NdPoint wfrom=Wsamples.logicToPixel(box.p1);
  NdPoint wto  =Wsamples.logicToPixel(box.p2); 
  NdPoint wstep=delta.rightShift(Wsamples.shift);

  NdPoint rfrom=Rsamples.logicToPixel(box.p1);
  NdPoint rto  =Rsamples.logicToPixel(box.p2);
  NdPoint rstep=delta.rightShift(Rsamples.shift);

  VisusAssert(NdPoint::max(wfrom,NdPoint(pdim))==wfrom); wto=NdPoint::min(wto,Wbuffer.dims);wstep=NdPoint::min(wstep,Wbuffer.dims);
  VisusAssert(NdPoint::max(rfrom,NdPoint(pdim))==rfrom); rto=NdPoint::min(rto,Rbuffer.dims);rstep=NdPoint::min(rstep,Rbuffer.dims);

  //first insert samples in the right position!
  if (!ArrayUtils::insert(Wbuffer,wfrom,wto,wstep,Rbuffer,rfrom,rto,rstep,aborted))
    return false;

  //eventually interpolate the samples got so far (NOTE: interpolate can be slow!)
  if (merge_mode==BaseQuery::InterpolateSamples && !ArrayUtils::interpolate(Wbuffer,wfrom,wto,wstep,aborted))
    return false;

  return true;
}


////////////////////////////////////////////////////////////////////////////////////
bool BaseQuery::allocateBufferIfNeeded() 
{
  if (!buffer)
  {
    if (!buffer.resize(nsamples,field.dtype,__FILE__,__LINE__)) 
      return false;
    buffer.fillWithValue(field.default_value);
  }
  else
  {
    //check buffer
    VisusAssert(buffer.dims==this->nsamples);
    VisusAssert(buffer.dtype==field.dtype);
    VisusAssert(buffer.c_size()==getByteSize());
  }

  return true;
}

} //namespace Visus

