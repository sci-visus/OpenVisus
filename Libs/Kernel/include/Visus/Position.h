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

  Matrix T= Matrix::identity(4);
  BoxNd  box;

  //constructor
  Position() {
  }

  //constructor
  Position(std::vector<Matrix> T, BoxNd box);

  //constructor
  Position(Matrix T, BoxNd box) : Position(std::vector<Matrix>({ T }), box) {
  }

  //constructor
  Position(BoxNd value) : Position(Matrix(), value) {
  }

  //constructor
  Position(BoxNi value) : Position(value.castTo<BoxNd>()) {
  }

  //constructor
  Position(const Matrix& T0, const Position& other) : Position(std::vector<Matrix>({ T0,other.T }), other.box) {
  }

  //constructor
  Position(const Matrix& T0, const Matrix& T1, const Position& other) : Position(std::vector<Matrix>({ T0,T1,other.T }), other.box) {
  }

  //constructor
  Position(const Matrix& T0, const Matrix& T1, const Matrix& T2, const Position& other) : Position(std::vector<Matrix>({ T0,T1,T1, other.T }), other.box) {
  }

  //invalid
  static Position invalid() {
    return Position();
  }

  //getPointDim
  int getPointDim() const {
    return box.getPointDim();
  }

  //setPointDim
  void setPointDim(int value) {
    box.setPointDim(value);
  }

  //getSpaceDim
  int getSpaceDim() const {
    return T.getSpaceDim();
  }

  //setSpaceDim
  void setSpaceDim(int value) {
    T.setSpaceDim(value);
  }

  //compose
  static Matrix compose(Position A, PointNi dims) {
    return
      A.T *
      Matrix::translate(A.box.p1) *
      Matrix::nonZeroScale(A.box.size()) *
      Matrix::invNonZeroScale(dims.castTo<PointNd>());
  }

  //operator!=
  bool operator==(const Position& other) const {
    return T==other.T && box==other.box;
  }

  //operator!=
  bool operator!=(const Position& other) const {
    return !(operator==(other));
  }

  //valid
  bool valid() const {
    return box.valid();
  }

  //getBoxNi
  BoxNi getBoxNi() const {
    return this->box.castTo<BoxNi>();
  }

  //computeVolume
  double computeVolume() const;

  //getPoints
  std::vector<PointNd> getPoints() const;

  //withoutTransformation
  BoxNd withoutTransformation() const {
    return this->valid() ? BoxNd(getPoints()) : BoxNd::invalid();
  }

  //shrink i.e. map * (position.lvalue,position.rvalue') should be in dst_box, does not change position.lvalue
  static Position shrink(BoxNd dst_box, LinearMap& map, Position position);

  //toString
  String toString() const {
    std::ostringstream out;
    out  << "T(" << T.toString() << ") box(" << box.toString() << ")";
    return out.str();
  }

public:

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) ;

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream) ;

};


} //namespace Visus

#endif //VISUS_POSITION_H


