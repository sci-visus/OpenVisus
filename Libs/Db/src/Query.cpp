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

#include <Visus/Query.h>
#include <Visus/Dataset.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////////
Query::Query(Dataset* dataset_,int mode) : dataset(dataset_)
{
  VisusAssert(mode=='r' || mode=='w');
  this->mode           = mode;
  this->time           = dataset->getDefaultTime();
  this->field          = dataset->getDefaultField();
  this->filter.domain  = dataset->getLogicBox();

  if (isPointQuery())
    this->point_query.coordinates = std::make_shared<HeapMemory>();
}

////////////////////////////////////////////////////////////////////////////////////
bool Query::isPointQuery() const {
  return dataset->getPointDim() == 3 && this->logic_position.getBoxNd().minsize() == 0;
}

////////////////////////////////////////////////////////////////////////////////////
void Query::setCurrentLevelReady()
{
  VisusAssert(status == QueryRunning);
  VisusAssert(this->buffer.dims == this->nsamples);
  VisusAssert(running_cursor >= 0 && running_cursor < end_resolutions.size());
  this->buffer.bounds = this->logic_position;
  this->buffer.clipping = this->logic_clipping;
  this->cur_resolution = end_resolutions[running_cursor];
}

////////////////////////////////////////////////////////////////////////////////////
bool Query::allocateBufferIfNeeded()
{
  if (!buffer)
  {
    if (!buffer.resize(nsamples, field.dtype, __FILE__, __LINE__))
      return false;
    buffer.fillWithValue(field.default_value);
  }
  else
  {
    //check buffer
    VisusAssert(buffer.dims == this->nsamples);
    VisusAssert(buffer.dtype == field.dtype);
    VisusAssert(buffer.c_size() == getByteSize());
  }

  return true;
}


//////////////////////////////////////////////////////////////////////////////////////////
bool Query::mergeSamples(LogicBox Wsamples, Array& Wbuffer, LogicBox Rsamples, Array Rbuffer, int merge_mode, Aborted aborted)
{
  if (!Wsamples.valid() || !Rsamples.valid())
    return false;

  //cannot find intersection (i.e. no sample to merge)
  BoxNi box = Wsamples.getIntersection(Rsamples);
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
  PointNi delta(pdim);
  for (int D = 0; D<pdim; D++)
  {
    Int64 lcm = Utils::leastCommonMultiple(Rsamples.delta[D], Wsamples.delta[D]);

    Int64 P1 = box.p1[D];
    Int64 P2 = box.p2[D];

    while (!Utils::isAligned(P1, Wsamples.p1[D], Wsamples.delta[D]) ||
      !Utils::isAligned(P1, Rsamples.p1[D], Rsamples.delta[D]))
    {
      //NOTE: if the value is already aligned, alignRight does nothing
      P1 = Utils::alignRight(P1, Wsamples.p1[D], Wsamples.delta[D]);
      P1 = Utils::alignRight(P1, Rsamples.p1[D], Rsamples.delta[D]);

      //cannot find any alignment, going beyond the valid range
      if (P1 >= P2)
        return false;

      //continue in the search IIF it's not useless
      if ((P1 - box.p1[D]) >= lcm)
      {
        //should be acceptable to be here, it just means that there are no samples to merge... 
        //but since 99% of the time Visus has pow-2 alignment it is high unlikely right now... adding the VisusAssert just for now
        VisusAssert(false);
        return false;
      }
    }

    delta[D] = lcm;
    P2 = Utils::alignRight(P2, P1, delta[D]);
    box.p1[D] = P1;
    box.p2[D] = P2;
  }

  VisusAssert(box.isFullDim());

  VisusAssert(Wbuffer.dims == Wsamples.nsamples);
  VisusAssert(Rbuffer.dims == Rsamples.nsamples);
  VisusAssert(Wbuffer.dtype == Rbuffer.dtype);

  PointNi wfrom = Wsamples.logicToPixel(box.p1);
  PointNi wto = Wsamples.logicToPixel(box.p2);
  PointNi wstep = delta.rightShift(Wsamples.shift);

  PointNi rfrom = Rsamples.logicToPixel(box.p1);
  PointNi rto = Rsamples.logicToPixel(box.p2);
  PointNi rstep = delta.rightShift(Rsamples.shift);

  VisusAssert(PointNi::max(wfrom, PointNi(pdim)) == wfrom); wto = PointNi::min(wto, Wbuffer.dims); wstep = PointNi::min(wstep, Wbuffer.dims);
  VisusAssert(PointNi::max(rfrom, PointNi(pdim)) == rfrom); rto = PointNi::min(rto, Rbuffer.dims); rstep = PointNi::min(rstep, Rbuffer.dims);

  //first insert samples in the right position!
  if (!ArrayUtils::insert(Wbuffer, wfrom, wto, wstep, Rbuffer, rfrom, rto, rstep, aborted))
    return false;

  //eventually interpolate the samples got so far (NOTE: interpolate can be slow!)
  if (merge_mode == Query::InterpolateSamples && !ArrayUtils::interpolate(Wbuffer, wfrom, wto, wstep, aborted))
    return false;

  return true;
}


} //namespace Visus

