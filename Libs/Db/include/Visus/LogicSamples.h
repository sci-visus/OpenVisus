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

#ifndef __VISUS_LOGIC_SAMPLES_H
#define __VISUS_LOGIC_SAMPLES_H

#include <Visus/Db.h>
#include <Visus/Array.h>

namespace Visus {


////////////////////////////////////////////////////////
class VISUS_DB_API LogicSamples 
{
public:

  VISUS_CLASS(LogicSamples)

  BoxNi   logic_box;
  PointNi nsamples;
  PointNi delta;
  PointNi shift;

  //default constructor
  LogicSamples() {
  }

  //constructor
  LogicSamples(BoxNi logic_box_, PointNi delta_)
    : logic_box(logic_box_),delta(delta_),shift(delta_.getLog2())
  {
    int pdim = logic_box.getPointDim();

    //a zero delta will cause problems. you can have an endless loop
    //  for (pos=;pos<=;pos+=delta)
    VisusAssert(delta.innerProduct()>0);

    //calculate how many samples I will get
    this->nsamples = PointNi::one(pdim);
    for (int D=0;D<pdim;D++)
      this->nsamples[D]=(logic_box.p2[D]- logic_box.p1[D])/ this->delta[D];

    //probably overflow
    if (this->nsamples.innerProduct()<=0 || !logic_box.isFullDim()) {
      *this= LogicSamples();
      return;
    }

    //check alignment
    VisusAssert(
      ((PointNi::one(pdim).leftShift(this->shift))==this->delta) && 
      (this->pixelToLogic(PointNi(pdim))==logic_box.p1) &&
      (this->pixelToLogic(nsamples)== logic_box.p2)  &&
      (this->logicToPixel(logic_box.p1)==PointNi(pdim)) &&
      (this->logicToPixel(logic_box.p2)==nsamples));
  }

  //invalid
  static LogicSamples invalid() {
    return LogicSamples();
  }

  //valid
  bool valid() const {
    return nsamples.innerProduct() > 0;
  }

  //operator==
  bool operator==(const LogicSamples& other) const {
    return logic_box == other.logic_box && nsamples == other.nsamples && delta == other.delta && shift == other.shift;
  }

  //operator!=
  bool operator!=(const LogicSamples& other) const  {
    return !(operator==(other));
  }

  //pixelToLogic
  inline PointNi pixelToLogic(const PointNi& value) const {
    return logic_box.p1 + value.leftShift(shift);
  }

  //logicToPixel
  inline PointNi logicToPixel(const PointNi& value) const {
    return (value- logic_box.p1).rightShift(shift);
  }

  //alignBox
  BoxNi alignBox(BoxNi value) const
  {
    int pdim = nsamples.getPointDim();

    if (!this->valid())
      return BoxNi::invalid();

    value= value.getIntersection(this->logic_box);

    if (!value.isFullDim())
      return BoxNi::invalid();

    //NOTE: i can move P2 to the right, since p2 is not included it it's aligned it wont' be moved
    //      if it's not aligned will be moved to the right, but all the extra samples wont't be "good" due to the delta
    for (int D = 0; D < pdim; D++) 
    {
      value.p1[D] = Utils::alignRight(value.p1[D], logic_box.p1[D], this->delta[D]);
      value.p2[D] = Utils::alignRight(value.p2[D], logic_box.p1[D], this->delta[D]);
    }
    return value;
  }


};

} //namespace Visus

#endif //__VISUS_LOGIC_SAMPLES_H

