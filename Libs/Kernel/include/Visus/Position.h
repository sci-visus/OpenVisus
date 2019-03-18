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

#ifndef VISUS_POSITION_H
#define VISUS_POSITION_H

#include <Visus/Kernel.h>
#include <Visus/Matrix.h>
#include <Visus/Box.h>

namespace Visus {

/////////////////////////////////////////////////
class VISUS_KERNEL_API Position 
{
public:

  VISUS_CLASS(Position)

  //constructor
  Position() {
  }

  //constructor
  Position(Box3d value) {
    if (!value.valid()) return;
    this->box = value;
    this->pdim = value.minsize() > 0 ? 3 : 2;
  }

  //constructor
  Position(NdBox value) {
    
    if (!value.isFullDim()) return;
    this->pdim = value.getPointDim();

    auto p1 = convertTo<PointNd>(value.p1);
    auto p2 = convertTo<PointNd>(value.p2);

    if ((p2[0] - p1[0]) == 1) p2[0] -= 1.0;
    if ((p2[1] - p1[1]) == 1) p2[1] -= 1.0;
    if ((p2[2] - p1[2]) == 1) p2[2] -= 1.0;
    if ((p2[3] - p1[3]) == 1) p2[3] -= 1.0;
    if ((p2[4] - p1[4]) == 1) p2[4] -= 1.0;

    this->box = BoxNd(p1, p2);

    VisusAssert(getNdBox()==value); //I should not loose any integer precision
  }

  //constructor
  Position(const Matrix& T0, const Position& other) : Position(other) {
    this->T = T0 * this->T;
  }

  //constructor
  Position(const Matrix& T0, const Matrix& T1, const Position& other) : Position(T1,other) {
    this->T = T0 * this->T;
  }

  //constructor
  Position(const Matrix& T0, const Matrix& T1, const Matrix& T2, const Position& other) : Position(T1,T2,other) {
    this->T = T0 * this->T;
  }

  //invalid
  static Position invalid() {
    return Position();
  }

  //getPointDim
  int getPointDim() const {
    return pdim;
  }

  //operator!=
  bool operator==(const Position& other) const {
    return pdim==other.pdim && T==other.T && box==other.box;
  }

  //operator!=
  bool operator!=(const Position& other) const {
    return !(operator==(other));
  }

  //valid
  bool valid() const {
    return pdim>0;
  }

  //getTransformation
  Matrix getTransformation() const {
    return T;
  }

  //setTransformation
  void setTransformation(const Matrix& value) {
    this->T=value;
  }

  //getBox
  Box3d getBox() const {
    return box.toBox3();
  }

  //getNdBox
  NdBox getNdBox() const {
    
    if (!valid())
      return NdBox::invalid(2);

    auto p1 = convertTo<PointNi>(box.p1);
    auto p2 = convertTo<PointNi>(box.p2);

    if ((p2[0] - p1[0]) == 0) p2[0] += 1;
    if ((p2[1] - p1[1]) == 0) p2[1] += 1;
    if ((p2[2] - p1[2]) == 0) p2[2] += 1;
    if ((p2[3] - p1[3]) == 0) p2[3] += 1;
    if ((p2[4] - p1[4]) == 0) p2[4] += 1;

    p1.setPointDim(this->pdim);
    p2.setPointDim(this->pdim);

    return NdBox(p1, p2);
  }

  //withoutTransformation
  Position withoutTransformation() const;  

  //toAxisAlignedBox
  Box3d toAxisAlignedBox() const {
    return withoutTransformation().getBox();
  }

  //shrink i.e. map * (position.lvalue,position.rvalue') should be in dst_box, does not change position.lvalue
  static Position shrink(const Box3d& dst_box, const LinearMap& map, Position position);

  //toString
  String toString() const {
    std::ostringstream out;
    out  << "T(" << getTransformation().toString() << ") box(" << getBox().toString() << ")";
    return out.str();
  }

public:

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) ;

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream) ;

private:

  int    pdim = 0;
  Matrix T;
  BoxNd  box;

};



} //namespace Visus

#endif //VISUS_POSITION_H


