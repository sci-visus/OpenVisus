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
  Polygon2d(const Matrix& H,const Polygon2d& other)   {
    VisusAssert(H.getSpaceDim()==3);
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
  BoxNd getBoundingBox() const {
    BoxNd ret= BoxNd::invalid();
    for (auto p:points) 
      ret.addPoint(p);
    return ret;
  }

  //translate
  Polygon2d translate(Point2d vt) const {
    return Polygon2d(Matrix::translate(vt),*this);
  }

  //scale
  Polygon2d scale(Point2d vs) const {
    return Polygon2d(Matrix::scale(vs),*this);
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


//////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Quad : public Polygon2d
{
public:

  //constructor
  Quad() {
    points.resize(4);
  }

  //constructor
  Quad(const Matrix& H, const Quad& q) : Polygon2d(H, q.points) {
    VisusAssert(H.getSpaceDim() == 3);
  }

  //constructor
  Quad(Point2d p0, Point2d p1, Point2d p2, Point2d p3) : Polygon2d(p0, p1, p2, p3) {
  }

  //constructor
  Quad(std::vector<Point2d> v) : Polygon2d(v[0], v[1], v[2], v[3]) {
    VisusAssert(points.size() == 4);
  }

  //constructor
  Quad(const Matrix& H, std::vector<Point2d> points) : Quad(H, Quad(points)) {
  }

  //constructor
  Quad(const Matrix& H, const BoxNd& box) {
    std::vector< Point2d> points;
    for (auto it : box.getPoints())
      points.push_back(it.toPoint2());
    *this = Quad(H, points);
  }

  //constructor
  Quad(int X, int Y, int W, int H) : Polygon2d(Point2d(X, Y), Point2d(X + W, Y), Point2d(X + W, Y + H), Point2d(X, Y + H)) {
  }

  //constructor
  Quad(int W, int H) : Polygon2d(Point2d(0, 0), Point2d(W, 0), Point2d(W, H), Point2d(0, H)) {
  }

  //centroid
  Point2d centroid() const {
    return Polygon2d::centroid();
  }

  //getPoint
  Point2d getPoint(int index) const {
    return points[index];
  }

  //translate
  Quad translate(Point2d vt) const {
    return Quad(points[0] + vt, points[1] + vt, points[2] + vt, points[3] + vt);
  }

  //computeBoundingBox
  static BoxNd computeBoundingBox(const std::vector<Quad>& quads)
  {
    auto ret = BoxNd::invalid();
    for (int I = 0; I<(int)quads.size(); I++)
      ret = ret.getUnion(quads[I].getBoundingBox());
    return ret;
  };

  //findQuadHomography
  static Matrix findQuadHomography(const Quad& dst, const Quad& src);

  //FindQuadIntersection
  static Polygon2d intersection(const Quad& A, const Quad& B);

  //isConvex
  bool isConvex() const;

  //toString
  String toString() const {
    return StringUtils::format()
      << points[0][0] << " " << points[0][1] << " "
      << points[1][0] << " " << points[1][1] << " "
      << points[2][0] << " " << points[2][1] << " "
      << points[3][0] << " " << points[3][1];
  }

  //toString
  static Quad fromString(String s) {
    Quad ret;
    std::istringstream parser(s);
    parser 
      >> ret.points[0][0] >> ret.points[0][1]
      >> ret.points[1][0] >> ret.points[1][1]
      >> ret.points[2][0] >> ret.points[2][1]
      >> ret.points[3][0] >> ret.points[3][1];
    return ret;
  }


};


} //namespace Visus

#endif //VISUS_POLYGON_H


