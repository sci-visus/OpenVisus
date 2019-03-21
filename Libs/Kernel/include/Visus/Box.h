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

#ifndef VISUS_BOX_H
#define VISUS_BOX_H

#include <Visus/Kernel.h>
#include <Visus/Plane.h>

#include <algorithm>

namespace Visus {

///////////////////////////////////////////////////////////////////
template <typename T>
class Box3
{
public:

  typedef Point3<T> Point;

  //points (see valid() function)
  Point p1, p2;

  //constructor
  Box3() {
  }

  //constructor
  Box3(Point p1_, Point p2_) : p1(p1_), p2(p2_) {
  }

  //return an invalid box
  static Box3 invalid()
  {
    T L = NumericLimits<T>::lowest();
    T H = NumericLimits<T>::highest();
    return Box3(Point(H, H, H), Point(L, L, L));
  }

  //valid (note: an axis can have zero dimension, trick to store slices too)
  bool valid() const{
    return p1.valid() && p2.valid() && p1.x <= p2.x && p1.y <= p2.y  && p1.z <= p2.z;
  }

  //center
  Point center() const {
    return 0.5*(p1 + p2);
  }

  //size
  Point size() const {
    return p2 - p1;
  }

  //max size
  T maxsize() const {
    Point d = size(); return Utils::max(d.x, d.y, d.z);
  }

  //min size
  T minsize() const {
    Point d = size(); return Utils::min(d.x, d.y, d.z);
  }

  //middle
  Point middle() const {
    return 0.5*(p1 + p2);
  }

  //addPoint
  void addPoint(Point p) {
    this->p1 = Point::min(this->p1, p);
    this->p2 = Point::max(this->p2, p);
  }

  //get point
  Point getPoint(int idx) const
  {
    switch (idx)
    {
    case 0:return Point(p1.x, p1.y, p1.z);
    case 1:return Point(p2.x, p1.y, p1.z);
    case 2:return Point(p2.x, p2.y, p1.z);
    case 3:return Point(p1.x, p2.y, p1.z);
    case 4:return Point(p1.x, p1.y, p2.z);
    case 5:return Point(p2.x, p1.y, p2.z);
    case 6:return Point(p2.x, p2.y, p2.z);
    case 7:return Point(p1.x, p2.y, p2.z);
    }
    VisusAssert(false);
    return Point();
  }

  //getPoints
  std::vector<Point> getPoints() const {
    return std::vector<Point>({
      getPoint(0),getPoint(1),getPoint(2),getPoint(3),getPoint(4),getPoint(5),getPoint(6),getPoint(7)
    });
  }

  //getPoint
  Point getPoint(double alpha, double beta, double gamma) const {
    return p1 + Point(alpha*(p2.x - p1.x), beta*(p2.y - p1.y), gamma*(p2.z - p1.z));
  }

  //test if a point is inside the box
  bool containsPoint(Point p) const {
    return this->p1 <= p && p <= this->p2;
  }

  //test if two box are equal
  bool operator==(const Box3& b) const {
    return p1 == b.p1 && p2 == b.p2;
  }

  //test equality
  bool operator!=(const Box3& b) const {
    return !(this->operator==(b));
  }

  //intersect
  bool intersect(const Box3& other) const {
    return valid() && other.valid() ? (p1 <= other.p2 && p2 >= other.p1) : false;
  }

  //get intersection of two boxes
  Box3 getIntersection(const Box3& b) const {
    Box3 ret;
    ret.p1 = Point::max(this->p1, b.p1);
    ret.p2 = Point::min(this->p2, b.p2);
    return ret;
  }

  //get union of two boxes
  Box3 getUnion(const Box3& b) const {
    Box3 ret;
    ret.p1 = Point::min(this->p1, b.p1);
    ret.p2 = Point::max(this->p2, b.p2);
    return ret;
  }

  //return the planes (pointing outside)
  std::vector<Plane> getPlanes() const
  {
    std::vector<Plane> ret;
    ret.reserve(6);
    ret.push_back(Plane(-1, 0., 0., +this->p1.x)); //x<=box.p1.x
    ret.push_back(Plane(+1, 0., 0., -this->p2.x)); //x>=box.p2.x
    ret.push_back(Plane(0., -1., 0., +this->p1.y)); //y<=box.p1.y
    ret.push_back(Plane(0., +1., 0., -this->p2.y)); //y>=box.p2.y
    ret.push_back(Plane(0., 0., -1., +this->p1.z)); //z<=box.p1.z
    ret.push_back(Plane(0., 0., +1., -this->p2.z)); //z>=box.p2.z
    return ret;
  }

  //scaleAroundCenter
  Box3 scaleAroundCenter(double scale)
  {
    Point center = this->center();
    Point size = scale*(p2 - p1);
    return Box3(center - size*0.5, center + size*0.5);
  }

public:

  //construct from string
  static Box3 parseFromString(String value)
  {
    Box3 ret;
    std::istringstream parser(value);
    parser >> ret.p1.x >> ret.p1.y >> ret.p1.z;
    parser >> ret.p2.x >> ret.p2.y >> ret.p2.z;
    return ret;
  }

  //construct to string
  String toString() const {
    return p1.toString() + " " + p2.toString();
  }

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) 
  {
    ostream.write("p1", p1.toString());
    ostream.write("p2", p2.toString());
  }

  //writeToObjectStream
  void readFromObjectStream(ObjectStream& istream) 
  {
    p1 = Point(istream.read("p1"));
    p2 = Point(istream.read("p2"));
  }

}; //end class Box3

typedef Box3<double> Box3d;

///////////////////////////////////////////////////////////////////
template <typename T>
class BoxN 
{
public:
  
  typedef PointN<T> Point;

  //points (see valid() function)
  Point p1, p2;

  //constructor
  BoxN() {
  }

  //copy constructor (needed by swig)
  BoxN(const BoxN& other) : p1(other.p1), p2(other.p2) {
}

  //constructor
  BoxN(int pdim) : p1(pdim), p2(pdim) {
  }

  //constructor
  BoxN(Point p1_, Point p2_) : p1(p1_), p2(p2_) {
    VisusAssert(p1.getPointDim() == p2.getPointDim());
  }

  //constructor
  BoxN(Box3<T> box) : p1(box.p1), p2(box.p2) {
  }

  //getPointDim
  int getPointDim() const {
    return p1.getPointDim();
  }

  //return an invalid box
  static BoxN invalid(int pdim) {
    T L = NumericLimits<T>::lowest();
    T H = NumericLimits<T>::highest();
    return BoxN(Point(pdim,H, H, H, H, H), Point(pdim, L, L, L, L, L));
  }

  //valid
  bool valid() const {
    return getPointDim()>0 && p1 <= p2;
  }

  //isFullDim
  bool isFullDim() const {
    return getPointDim()>0 && p1<p2;
  }

  //center
  Point center() const {
    return 0.5*(p1 + p2);
  }

  //size
  Point size() const {
    return p2 - p1;
  }

  //max size
  T maxsize() const {
    return size().maxsize();
  }

  //min size
  T minsize() const {
    return size().minsize();
  }

  //middle
  Point middle() const {
    return 0.5*(p1 + p2);
  }

  //addPoint
  void addPoint(Point p) {
    this->p1 = Point::min(this->p1, p);
    this->p2 = Point::max(this->p2, p);
  }

  //toBox3
  Box3<T> toBox3() const {
    return Box3<T>(p1.toPoint3(), p2.toPoint3());
  }

  //test if a point is inside the box
  bool containsPoint(Point p) const {
    return this->p1 <= p && p <= this->p2;
  }

  //test if two box are equal
  bool operator==(const BoxN& b) const {
    return p1 == b.p1 && p2 == b.p2;
  }

  //test equality
  bool operator!=(const BoxN& b) const {
    return !(this->operator==(b));
  }

  //intersect
  bool intersect(const BoxN& other) const {
    return valid() && other.valid() ? (p1 <= other.p2 && p2 >= other.p1) : false;
  }

  //strictIntersect
  bool strictIntersect(const BoxN& other) const {
    return valid() && other.valid() ? (p1<other.p2 && p2>other.p1) : false;
  }

  //get intersection of two boxes
  BoxN getIntersection(const BoxN& b) const {
    const BoxN& a = *this;
    if (!a.valid()) return a;
    if (!b.valid()) return b;
    BoxN ret;
    ret.p1 = Point::max(a.p1, b.p1);
    ret.p2 = Point::min(a.p2, b.p2);
    return ret;
  }

  //get union of two boxes
  BoxN getUnion(const BoxN& b) const {
    const BoxN& a = *this;
    if (!a.valid()) return b;
    if (!b.valid()) return a;
    BoxN ret;
    ret.p1 = Point::min(a.p1, b.p1);
    ret.p2 = Point::max(a.p2, b.p2);
    return ret;
  }

  //containsBox
  bool containsBox(const BoxN& other) const {
    return p1 <= other.p1 && other.p2 <= p2;
  }

  //scaleAroundCenter
  BoxN scaleAroundCenter(double scale)
  {
    Point center = this->center();
    Point size = scale*(p2 - p1);
    return BoxN(center - size*0.5, center + size*0.5);
  }

  //getSlab
  BoxN getSlab(int axis, T v1, T v2) const {
    return BoxN(p1.withValueOnAxis(axis, v1), p2.withValueOnAxis(axis, v2));
  }

  BoxN getXSlab(T x1, T x2) const { return getSlab(0, x1, x2); }
  BoxN getYSlab(T y1, T y2) const { return getSlab(1, y1, y2); }
  BoxN getZSlab(T z1, T z2) const { return getSlab(2, z1, z2); }

  //translate
  BoxN translate(const Point& vt) const {
    return BoxN(p1 + vt, p2 + vt);
  }

public:

  //construct from string
  static BoxN parseFromString(String value)
  {
    std::istringstream parser(value);
    std::vector<T> v1, v2;
    T value1, value2;
    while (parser >> value1 >> value2)
    {
      v1.push_back(value1);
      v2.push_back(value2);
    }

    return BoxN(Point(v1),Point(v2));
  }

  //construct to string
  String toString() const  
  {
    int pdim = getPointDim();
    if (!pdim) return "";

    std::ostringstream out;
    if (pdim >= 1) out <<        p1[0] << " " << p2[0];
    if (pdim >= 2) out << " " << p1[1] << " " << p2[1];
    if (pdim >= 3) out << " " << p1[2] << " " << p2[2];
    if (pdim >= 4) out << " " << p1[3] << " " << p2[3];
    if (pdim >= 5) out << " " << p1[4] << " " << p2[4];
    return out.str();
  }

  //toOldFormatString 
  String toOldFormatString() const
  {
    auto tmp = (*this);
    tmp.p2 = tmp.p2 - Point::one(getPointDim());
    return tmp.toString();
  }

  //parseFromOldFormatString
  static BoxN parseFromOldFormatString(int pdim,String src)
  {
    auto tmp = BoxN::parseFromString(src);
    tmp.p1.setPointDim(pdim);
    tmp.p2.setPointDim(pdim);
    tmp.p2 += Point::one(pdim);
    return tmp;
  }

  //writeToObjectStream`
  void writeToObjectStream(ObjectStream& ostream) 
  {
    ostream.write("p1", p1.toString());
    ostream.write("p2", p2.toString());
  }

  //writeToObjectStream
  void readFromObjectStream(ObjectStream& istream) 
  {
    p1 = Point::parseFromString(istream.read("p1"));
    p2 = Point::parseFromString(istream.read("p2"));
  }

}; //end class BoxN

typedef BoxN<double> BoxNd;
typedef BoxN<Int64>  BoxNi;

//backward compatible
typedef BoxNi NdBox;


} //namespace Visus

#endif //VISUS_BOX_H


