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

#ifndef VISUS_POLYGON_H
#define VISUS_POLYGON_H

#include <Visus/Kernel.h>
#include <Visus/Rectangle.h>
#include <Visus/Matrix.h>
#include <Visus/Box.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Polygon2d
{
public:

  VISUS_CLASS(Polygon2d)

  std::vector<Point2d> points;

  //constructor
  Polygon2d() {
  }

  //constructor
  Polygon2d(const std::vector<Point2d>& points_) : points(points_) {
  }

  //constructor
  Polygon2d(Point2d p0,Point2d p1,Point2d p2) : points(std::vector<Point2d>({p0,p1,p2})){
  }

  //constructor
  Polygon2d(Point2d p0,Point2d p1,Point2d p2,Point2d p3) : points(std::vector<Point2d>({p0,p1,p2,p3})){
  }

  //constructor
  Polygon2d(int W,int H) : Polygon2d(Point2d(0,0),Point2d(W,0),Point2d(W,H),Point2d(0,H)) {
  }

  //constructor
  Polygon2d(const Matrix3& H,const Polygon2d& other)   {
    for (auto p : other.points)
      points.push_back((H*Point3d(p,1)).dropHomogeneousCoordinate());
  }

  //valid
  bool valid() const {
    return points.size()>=3;
  }

  //operator==
  bool operator==(const Polygon2d& q) const {
    return points==q.points;
  }

  //operator!=
  bool operator!=(const Polygon2d& q) const {
    return points!=q.points;
  }

  //getBoundingBox
  Box3d getBoundingBox() const {
    Box3d ret= Box3d::invalid();
    for (auto p:points) 
      ret.addPoint(Point3d(p,0));
    return ret;
  }

  //translate
  Polygon2d translate(Point2d vt) const {
    return Polygon2d(Matrix3::translate(vt),*this);
  }

  //scale
  Polygon2d scale(Point2d vs) const {
    return Polygon2d(Matrix3::scale(vs),*this);
  }

  //centroid
  Point2d centroid() const {
    Point2d ret;
    for (auto it : points)
      ret+=it;
    ret*=(1.0/(double)points.size());
    return ret;
  }

  //toString
  String toString() const {
    std::ostringstream out;
    out<<"[";
    for (int I=0;I<(int)points.size();I++)
      out<<(I?", ":"")<<points[I].toString();
    out<<"]";
    return out.str();
  }

  //area
  double area() const;

  //clip
  Polygon2d clip(const Rectangle2d& r) const;

};

} //namespace Visus

#endif //VISUS_POLYGON_H


