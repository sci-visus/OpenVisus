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

#ifndef VISUS_POINT_H
#define VISUS_POINT_H

#include <Visus/Kernel.h>
#include <Visus/Utils.h>

#include <array>
#include <algorithm>

namespace Visus {

////////////////////////////////////////////////////////////////////////
template <typename T>
class Point2
{
public:

  typedef T coord_t;

  std::array<T, 2> coords = std::array<T, 2>({ 0,0 });

  //default constructor
  Point2() {
  }

  //constructor
  explicit Point2(T a, T b) : coords({ a,b }) {
  }

  //constructor
  explicit Point2(const std::vector<T>& src) : Point2(src[0], src[1]) {
    VisusAssert(src.size() == 2);
  }

  //constructor from string
  explicit Point2(String value) {
    std::istringstream parser(value); parser >> coords[0] >> coords[1];
  }


  //castTo
  template <typename Other>
  Other castTo() const {
    return Other((typename Other::coord_t)get(0), (typename Other::coord_t)get(1));
  }

  //toVector
  std::vector<T> toVector() const {
    return std::vector<T>(coords.begin(),coords.end());
  }

  //module*module
  T module2() const {
    return coords[0]*coords[0] + coords[1]*coords[1];
  }

  //module
  double module() const {
    return std::sqrt((double)module2());
  }

  //distance between two points
  double distance(const Point2& p) const {
    return (p - *this).module();
  }

  //normalize a vector
  Point2 normalized() const
  {
    T len = module();
    if (!len) len = 1.0;
    return Point2(coords[0] / len, coords[1] / len);
  }

  //abs
  Point2 abs() const {
    return Point2(coords[0] >= 0 ? +coords[0] : -coords[0], coords[1] >= 0 ? +coords[1] : -coords[1]);
  }

  //inv
  Point2 inv() const {
    return Point2((T)(1.0 / coords[0]), (T)(1.0 / coords[1]));
  }

  //-a
  Point2 operator-()  const {
    return Point2(-this->coords[0], -this->coords[1]);
  }

  //a+b
  Point2 operator+(const Point2&  b)  const {
    return Point2(this->coords[0] + b.coords[0], this->coords[1] + b.coords[1]);
  }

  //a+=b
  Point2& operator+=(const Point2&  b) {
    this->coords[0] += b.coords[0]; this->coords[1] += b.coords[1]; return *this;
  }

  //a-b
  Point2 operator-(const Point2&  b)  const {
    return Point2(this->coords[0] - b.coords[0], this->coords[1] - b.coords[1]);
  }

  //a-=b
  Point2& operator-=(const Point2&  b) {
    this->coords[0] -= b.coords[0]; this->coords[1] -= b.coords[1]; return *this;
  }

  //a*f
  template <typename Value>
  Point2 operator*(Value value) const {
    return Point2((T)(this->coords[0]*value), (T)(this->coords[1]*value));
  }

  //a*=f
  Point2& operator*=(T s) {
    this->coords[0] = this->coords[0]*s; this->coords[1] = this->coords[1]*s; return *this;
  }

  //a==b
  bool operator==(const Point2& b) const {
    return  coords[0] == b.coords[0] && coords[1] == b.coords[1];
  }

  //a!=b
  bool operator!=(const Point2& b) const {
    return !(operator==(b));
  }

  //dot product
  T dot(const Point2&  b) const {
    return this->coords[0]*b.coords[0] + this->coords[1]*b.coords[1];
  }

  //access an item using an index
  T& get(int i) {
    return coords[i];
  }

  //access an item using an index
  const T& get(int i) const {
    return coords[i];
  }

  //access an item using an index
  T& operator[](int i) {
    return get(i);
  }

  //access an item using an index
  const T& operator[](int i) const {
    return get(i);
  }

  //test if numers are ok
  bool valid() const
  {
    return
      Utils::isValidNumber(coords[0]) &&
      Utils::isValidNumber(coords[1]);
  }

  // return index of smallest/largest value
  int biggest () const { return (coords[0] >  coords[1]) ? (0) : (1); }
  int smallest() const { return (coords[0] <= coords[1]) ? (0) : (1); }

  //innerMultiply
  inline Point2 innerMultiply(const Point2& other) const {
    return Point2(coords[0] *other.coords[0], coords[1]*other.coords[1]);
  }

  //innerDiv
  inline Point2 innerDiv(const Point2& other) const {
    return Point2(coords[0] / other.coords[0], coords[1] / other.coords[1]);
  }

public:

  //min,max 
  static Point2 min(const Point2& a, const Point2& b) { return Point2(std::min(a[0], b[0]), std::min(a[1], b[1])); }
  static Point2 max(const Point2& a, const Point2& b) { return Point2(std::max(a[0], b[0]), std::max(a[1], b[1])); }

  //operator (<,<=,>,>=)
  bool operator< (const Point2& b) const { const Point2& a = *this; return a[0] <  b[0] && a[1] <  b[1]; }
  bool operator<=(const Point2& b) const { const Point2& a = *this; return a[0] <= b[0] && a[1] <= b[1]; }
  bool operator> (const Point2& b) const { const Point2& a = *this; return a[0] >  b[0] && a[1] >  b[1]; }
  bool operator>=(const Point2& b) const { const Point2& a = *this; return a[0] >= b[0] && a[1] >= b[1]; }


public:

  //convert to string
  String toString() const
  {
    std::ostringstream out;
    out << this->coords[0] << " " << this->coords[1];
    return out.str();
  }

  //operator<<
#if !SWIG
  friend std::ostream& operator<<(std::ostream &out, const Point2& p) {
    out << "<" << p.coords[0] << "," << p.coords[1] << ">"; return out;
  }
#endif

};//end class Point2


template <typename Value, typename T>
inline Point2<T> operator*(Value value, const Point2<T>& v) {
  return v * value;
}

typedef  Point2<Int64>  Point2i;
typedef  Point2<float>  Point2f;
typedef  Point2<double> Point2d;

////////////////////////////////////////////////////////////////////////
template <typename T>
class Point3
{
public:

  typedef T coord_t;

  std::array<T, 3> coords = std::array<T, 3>({ 0,0,0 });

  //default constructor
  Point3() {
  }

  //constructor
  explicit Point3(T a, T b, T c = T(0)) :coords({a,b,c}) {
  }

  //constructor
  explicit Point3(const Point2<T>& src, T d = T(0)) : Point3(src.coords[0], src.coords[1],d) {
  }

  //constructor
  explicit Point3(const std::vector<T>& src) : Point3(src[0],src[1],src[2]) {
    VisusAssert(src.size() == 3);
  }

  //constructor from string
  explicit Point3(String value) {
    std::istringstream parser(value); parser >> coords[0] >> coords[1] >> coords[2];
  }

  //castTo
  template <typename Other>
  Other castTo() const {
    return Other((typename Other::coord_t)get(0), (typename Other::coord_t)get(1), (typename Other::coord_t)get(2));
  }


  //toVector
  std::vector<T> toVector() const {
    return std::vector<T>(coords.begin(), coords.end());
  }

  //toPoint2
  Point2<T> toPoint2() const {
    return Point2<T>(coords[0], coords[1]);
  }

  //dropHomogeneousCoordinate
  Point2<T> dropHomogeneousCoordinate() const {
    return Point2<T>(coords[0] / (coords[2]? coords[2]: 1.0), coords[1] / (coords[2] ? coords[2] : 1.0));
  }

  //module*module
  T module2() const {
    return coords[0]*coords[0] + coords[1]*coords[1] + coords[2]*coords[2];
  }

  //module
  T module() const {
    return (T)sqrt(module2());
  }

  //distance between two points
  T distance(const Point3& p) const {
    return (p - *this).module();
  }

  //normalize a vector
  Point3 normalized() const
  {
    T len = module();
    if (len==T(0)) len = T(1);
    return Point3((T)(coords[0] / len), (T)(coords[1] / len), (T)(coords[2] / len));
  }

  //abs
  Point3 abs() const {
    return Point3(coords[0] >= 0 ? +coords[0] : -coords[0], coords[1] >= 0 ? +coords[1] : -coords[1], coords[2] >= 0 ? +coords[2] : -coords[2]);
  }

  //inverse
  Point3 inv() const {
    return Point3((T)(1.0 / coords[0]), (T)(1.0 / coords[1]), (T)(1.0 / coords[2]));
  }

  //+a
  const Point3& operator+()  const {
    return *this;
  }

  //-a
  Point3 operator-()  const {
    return Point3(-this->coords[0], -this->coords[1], -this->coords[2]);
  }

  //a+b
  Point3 operator+(const Point3&  b)  const {
    return Point3(this->coords[0] + b.coords[0], this->coords[1] + b.coords[1], this->coords[2] + b.coords[2]);
  }

  //a+=b
  Point3& operator+=(const Point3&  b) {
    this->coords[0] += b.coords[0]; this->coords[1] += b.coords[1]; this->coords[2] += b.coords[2]; return *this;
  }

  //a-b
  Point3 operator-(const Point3&  b)  const {
    return Point3(this->coords[0] - b.coords[0], this->coords[1] - b.coords[1], this->coords[2] - b.coords[2]);
  }

  //a-=b
  Point3& operator-=(const Point3&  b) {
    this->coords[0] -= b.coords[0]; this->coords[1] -= b.coords[1]; this->coords[2] -= b.coords[2]; return *this;
  }

  //a*f
  template <typename Coeff>
  Point3 operator*(Coeff coeff) const {
    return Point3((T)(this->coords[0]* coeff), (T)(this->coords[1]* coeff), (T)(this->coords[2]*coeff));
  }

  //a*=f
  template <typename Value>
  Point3& operator*=(Value value) {
    this->coords[0] = this->coords[0]*value; this->coords[1] = this->coords[1]*value; this->coords[2] = this->coords[2]*value; return *this;
  }

  //a==b
  bool operator==(const Point3& b) const {
    return  coords[0] == b.coords[0] && coords[1] == b.coords[1] && coords[2] == b.coords[2];
  }

  //a!=b
  bool operator!=(const Point3& b) const {
    return  !(operator==(b));
  }

  //dot product
  T dot(const Point3&  b) const {
    return this->coords[0]*b.coords[0] + this->coords[1]*b.coords[1] + this->coords[2]*b.coords[2];
  }

  //access an item using an index
  T& get(int i) {
    return coords[i];
  }

  //access an item using an index
  const T& get(int i) const {
    return coords[i];
  }

  //access an item using an index
  T& operator[](int i) {
    return get(i);
  }

  //access an item using an index
  const T& operator[](int i) const {
    return get(i);
  }

  //set
  Point3& set(int index, T value) {
    (*this)[index] = value; return *this;
  }

  //cross product
  Point3 cross(const Point3& v) const {
    return Point3 (
      coords[1] * v.coords[2] - v.coords[1] * coords[2],
      coords[2] * v.coords[0] - v.coords[2] * coords[0],
      coords[0] * v.coords[1] - v.coords[0] * coords[1]
    );
  }

  //test if numers are ok
  bool valid() const
  {
    return
      Utils::isValidNumber(coords[0]) &&
      Utils::isValidNumber(coords[1]) &&
      Utils::isValidNumber(coords[2]);
  }

  // return index of smallest/largest value
  int biggest () const { return (coords[0] >  coords[1]) ? (coords[0] >  coords[2] ? 0 : 2) : (coords[1] >  coords[2] ? 1 : 2); }
  int smallest() const { return (coords[0] <= coords[1]) ? (coords[0] <= coords[2] ? 0 : 2) : (coords[1] <= coords[2] ? 1 : 2); }

  //min,max 
  static Point3 min(const Point3& a, const Point3& b) { return Point3(std::min(a[0], b[0]), std::min(a[1], b[1]), std::min(a[2], b[2])); }
  static Point3 max(const Point3& a, const Point3& b) { return Point3(std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2])); }

  //operator (<,<=,>,>=)
  bool operator< (const Point3& b) const { const Point3& a = *this; return a[0] <  b[0] && a[1] <  b[1] && a[2] <  b[2]; }
  bool operator<=(const Point3& b) const { const Point3& a = *this; return a[0] <= b[0] && a[1] <= b[1] && a[2] <= b[2]; }
  bool operator> (const Point3& b) const { const Point3& a = *this; return a[0] >  b[0] && a[1] >  b[1] && a[2] >  b[2]; }
  bool operator>=(const Point3& b) const { const Point3& a = *this; return a[0] >= b[0] && a[1] >= b[1] && a[2] >= b[2]; }


public:

  //convert to string
  String toString() const
  {
    std::ostringstream out;
    out << this->coords[0] << " " << this->coords[1] << " " << this->coords[2];
    return out.str();
  }


#if !SWIG
  //operator<<
  friend std::ostream& operator<<(std::ostream &out,const Point3& p) {
    out << "<" << p.coords[0] << "," << p.coords[1] << "," << p.coords[2] << ">"; return out;
  }
#endif

};//end class Point3


template <typename Coeff, typename T>
inline Point3<T> operator*(Coeff coeff, const Point3<T>& v) {
  return v * coeff;
}

typedef  Point3<Int64>  Point3i;
typedef  Point3<float>  Point3f;
typedef  Point3<double> Point3d;


///////////////////////////////////////////////////////////////////////////
template <typename T>
class Point4
{
public:

  typedef T coord_t;

  std::array<T, 4> coords = std::array<T, 4>({ 0,0,0,0 });

  //default constructor
  Point4() {
  }

  //constructor
  explicit Point4(T a, T b, T c = 0, T d = 0)
    : coords({ a,b,c,d } ) {
  }

  //constructor
  explicit Point4(const Point3<T>& src, T d = 0)
    : Point4(src.coords[0], src.coords[1], src.coords[2], d) {
  }

  //constructor
  explicit Point4(const std::vector<T> src) : Point4(src[0],src[1],src[2],src[3]) {
    VisusAssert(src.size() == 4);
  }

  //constructor from string
  explicit Point4(String value) {
    std::istringstream parser(value); parser >> coords[0] >> coords[1] >> coords[2] >> coords[3];
  }

  //castTo
  template <typename Other>
  Other castTo() const {
    return Other((typename Other::coord_t)get(0), (typename Other::coord_t)get(1), (typename Other::coord_t)get(2), (typename Other::coord_t)get(3));
  }

  //toVector
  std::vector<T> toVector() const {
    return std::vector<T>(coords.begin(), coords.end());
  }

  //toPoint3
  Point3<T> toPoint3() const {
    return Point3<T>(coords[0], coords[1], coords[2]);
  }

  //dropHomogeneousCoordinate
  Point3<T> dropHomogeneousCoordinate() const
  {
    T W = this->coords[3];
    if (!W) W = 1;
    return Point3<T>(coords[0] / W, coords[1] / W, coords[2] / W);
  }

  //module2
  T module2() const {
    return coords[0]*coords[0] + coords[1]*coords[1] + coords[2]*coords[2] + coords[3]*coords[3];
  }

  //module
  T module() const {
    return (T)sqrt((T)module2());
  }

  //distance between two points
  T distance(const Point4& p) const {
    return (p - *this).module();
  }

  //normalized
  Point4 normalized() const
  {
    T len = module();
    if (len==T(0)) return *this;
    auto vs = T(1) / len;
    return Point4(coords[0] * vs, coords[1] * vs, coords[2] * vs, coords[3] * vs);
  }

  //abs
  Point4 abs() const {
    return Point4(coords[0] >= 0 ? +coords[0] : -coords[0], coords[1] >= 0 ? +coords[1] : -coords[1], coords[2] >= 0 ? +coords[2] : -coords[2], coords[3] >= 0 ? +coords[3] : -coords[3]);
  }

  //inverse
  Point4 inv() const {
    return Point4((T)(1.0 / coords[0]), T(1.0 / coords[1]), (T)(1.0 / coords[2]), (T)(1.0 / coords[3]));
  }

  //operator-
  Point4 operator-()  const {
    return Point4(-coords[0], -coords[1], -coords[2], -coords[3]);
  }

  //a+b
  Point4 operator+(const Point4& b)  const {
    return Point4(coords[0] + b.coords[0], coords[1] + b.coords[1], coords[2] + b.coords[2], coords[3] + b.coords[3]);
  }

  //a+=b
  Point4& operator+=(const Point4&  b) {
    coords[0] += b.coords[0]; coords[1] += b.coords[1]; coords[2] += b.coords[2]; coords[3] += b.coords[3]; return *this;
  }

  //a-b
  Point4 operator-(const Point4&  b)  const{
    return Point4(coords[0] - b.coords[0], coords[1] - b.coords[1], coords[2] - b.coords[2], coords[3] - b.coords[3]);
  }

  //a-=b
  Point4& operator-=(const Point4&  b) {
    coords[0] -= b.coords[0]; coords[1] -= b.coords[1]; coords[2] -= b.coords[2]; coords[3] -= b.coords[3]; return *this;
  }

  //a*f
  template <typename Value>
  Point4 operator*(Value value) const {
    return Point4(coords[0]*value, coords[1]*value, coords[2]*value, coords[3]*value);
  }

  //a*=f
  template <typename Value>
  Point4& operator*=(Value value) {
    return (*this=*this * value);
  }

  //a==b
  bool operator==(const Point4& b) const {
    return  coords[0] == b.coords[0] && coords[1] == b.coords[1] && coords[2] == b.coords[2] && coords[3] == b.coords[3];
  }

  //a!=b
  bool operator!=(const Point4& b) const {
    return  !(operator==(b));
  }

  //dot product
  T dot(const Point4&  b) const {
    return coords[0]*b.coords[0] + coords[1]*b.coords[1] + coords[2]*b.coords[2] + coords[3]*b.coords[3];
  }

  //access an item using an index
  T& get(int i) {
    return coords[i];
  }

  //access an item using an index
  const T& get(int i) const {
    return coords[i];
  }

  //access an item using an index
  T& operator[](int i) {
    return get(i);
  }

  //access an item using an index
  const T& operator[](int i) const {
    return get(i);
  }

  //test if numers are ok
  bool valid() const
  {
    return
      Utils::isValidNumber(coords[0]) &&
      Utils::isValidNumber(coords[1]) &&
      Utils::isValidNumber(coords[2]) &&
      Utils::isValidNumber(coords[3]);
  }

public:

  //convert to string
  String toString() const
  {
    std::ostringstream out;
    out << this->coords[0] << " " << this->coords[1] << " " << this->coords[2] << " " << this->coords[3];
    return out.str();
  }


#if !SWIG
  //operator<<
  friend std::ostream& operator<<(std::ostream &out,const Point4& p)  {
    out << "<" << p.coords[0] << "," << p.coords[1] << "," << p.coords[2] << "," << p.coords[3] << ">"; return out;
  }
#endif

};//end class Point4


template <typename Value, typename T>
inline Point4<T> operator*(Value value, const Point4<T>& v) {
  return v * value;
}

typedef  Point4<Int64>  Point4i;
typedef  Point4<float>  Point4f;
typedef  Point4<double> Point4d;


//////////////////////////////////////////////////////////////
template <typename T>
class PointN
{
public:

  typedef T coord_t;

  //_____________________________________________________
  struct Compare
  {
    bool operator()(const PointN& a, const  PointN& b) const {
      return a.coords < b.coords;
    }
  };

  //_____________________________________________________
  class ForEachPoint
  {
  public:

    PointN pos;
    PointN from, to, step;

    int    pdim = 0;
    bool   bEnd = true;

    //constructor
    ForEachPoint(PointN from_, PointN to_, PointN step_) : pos(from_), from(from_), to(to_), step(step_), pdim(from_.getPointDim())
    {
      VisusAssert(pdim == from.getPointDim());
      VisusAssert(pdim == to.getPointDim());
      VisusAssert(pdim == step.getPointDim());
      VisusAssert(pdim == pos.getPointDim());

      if (!pdim)
        return;

      for (int D = 0; D < pdim; D++) {
        if (!(pos[D] >= from[D] && pos[D] < to[D]))
          return;
      }

      this->bEnd = false;
    }

    //end
    bool end() const {
      return bEnd;
    }

    //next
    void next()
    {
      if (!bEnd && ((pos[0] += step[0]) >= to[0]))
      {
        pos[0] = from[0];
        for (int D = 1; D < pdim; D++)
        {
          if ((pos[D] += step[D]) < to[D])
            return;
          pos[D] = from[D];
        }
        bEnd = true;
      }
    }
  };

  std::vector<T> coords;

  //default constructor
  PointN() {
  }

  //constructor
  explicit PointN(const std::vector<T>& coords_)
    : coords(coords_) {
  }

  //constructor
  explicit PointN(int pdim)
    : coords(std::vector<T>(pdim, T(0))) {
  }

  //constructor
  explicit PointN(const PointN& left, T right) :
    coords(left.coords) {
    this->coords.push_back(right);
  }

  //constructor
  explicit PointN(T a, T b)
    : coords({ a,b }) {
  }

  //constructor
  explicit PointN(T a, T b, T c)
    : coords({ a,b,c }) {
  }

  //constructor
  explicit PointN(T a, T b, T c,T d)
    : coords({ a,b,c,d }) {
  }

  //constructor
  explicit PointN(T a, T b, T c, T d,T e)
    : coords({ a,b,c,d,e }) {
  }

  //constructor
  PointN(Point2<T> p) : PointN(p.toVector()) {
  }

  //constructor
  PointN(Point3<T> p) : PointN(p.toVector()) {
  }

  //constructor
  PointN(Point4<T> p) : PointN(p.toVector()) {
  }

  //getPointDim
  int getPointDim() const {
    return (int)coords.size();
  }

  //push_back
  void push_back(T value) {
    this->coords.push_back(value);
  }

  //pop_back
  void pop_back() {
    this->coords.pop_back();
  }

  //withoutBack
  PointN withoutBack() const {
    auto ret = *this;
    ret.pop_back();
    return ret;
  }

  //back
  T back() const {
    return this->coords.back();
  }

  //back
#if !SWIG
  T& back() {
    return this->coords.back();
  }
#endif

  //dropHomogeneousCoordinate
  PointN dropHomogeneousCoordinate() const {
    return applyOperation(*this, MulByCoeff<double>(1.0 / back())).withoutBack();
  }

  //castTo
  template <typename Other>
  Other castTo() const {
    auto pdim = (int)coords.size();
    auto ret = Other(pdim);
    for (int I = 0; I < pdim; I++)
      ret[I] = (typename Other::coord_t)get(I);
    return ret;
  }

  //setPointDim
  void setPointDim(int new_pdim, T default_value = 0.0) {
    this->coords.resize(new_pdim, default_value);
  }

  //constructor
  static PointN one(int pdim) {
    return PointN(std::vector<T>(pdim, T(1)));
  }

  //one
  static PointN one(T a, T b) {
    return PointN(a,b);
  }

  //one
  static PointN one(T a, T b, T c) {
    return PointN(a,b,c);
  }

  //toVector
  const std::vector<T>& toVector() const {
    return coords;
  }

  //test if numers are ok
  bool valid() const {
    return checkAll<ConditionValidNumber>(*this);
  }

  //get
  T& get(int i) {
    return coords[i];
  }

  //const
  const T& get(int i) const {
    return coords[i];
  }

  //operator[]
  const T& operator[](int i) const {
    return get(i);
  }

  //operator[]
  T& operator[](int i) {
    return get(i);
  }

  //operator-
  PointN operator-()  const {
    return applyOperation<NegOp>(*this);
  }

  //a+b
  PointN operator+(const PointN& other)  const {
    return applyOperation<AddOp>(*this, other);
  }

  //a-b
  PointN operator-(const PointN& other)  const {
    return applyOperation<SubOp>(*this, other);
  }

  //a*s
  template <typename Coeff>
  PointN operator*(Coeff coeff) const {
    return applyOperation(*this, MulByCoeff<Coeff>(coeff));
  }

  //a+=b
  PointN& operator+=(const PointN& other) {
    return ((*this) = (*this) + other);
  }

  //a-=b
  PointN& operator-=(const PointN& other) {
    return ((*this) = (*this) - other);
  }

  //a*=s
  PointN& operator*=(T s) {
    return ((*this) = (*this) * s);
  }

  //a==b
  bool operator==(const PointN& other) const {
    return coords == other.coords;;
  }

  //a!=b
  bool operator!=(const PointN& other) const {
    return coords != other.coords;;
  }

  //min
  static PointN min(const PointN& a, const PointN& b) {
    return applyOperation<MinOp>(a, b);
  }

  //max
  static PointN max(const PointN& a, const PointN& b) {
    return applyOperation< MaxOp>(a, b);
  }

  //module2
  T module2() const {
    return this->dot(*this);
  }

  //module
  double module() const {
    return std::sqrt((double)module2());
  }

  //distance between two points
  double distance(const PointN& p) const {
    return (p - *this).module();
  }

  //normalized
  PointN normalized() const
  {
    T len = module();
    if (!len) len = 1.0;
    return (*this) * (1.0/len);
  }

  //abs
  PointN abs() const {
    return applyOperation<AbsOp>(*this);
  }

  //inv
  PointN inv() const {
    return applyOperation<InvOp>(*this);  
  }

  //minsize
  T minsize() const {
    return Utils::min(this->coords);
  }

  //maxsize
  T maxsize() const {
    return Utils::max(this->coords);
  }

  //operator (<,<=,>,>=) (NOTE: it's different from lexigraphical order)
  bool operator< (const PointN& b) const { return checkAll< ConditionL >(*this, b); }
  bool operator<=(const PointN& b) const { return checkAll< ConditionLE>(*this, b); }
  bool operator> (const PointN& b) const { return checkAll< ConditionG >(*this, b); }
  bool operator>=(const PointN& b) const { return checkAll< ConditionGE>(*this, b); }

public:

  //dot product
  T dot(const PointN& other) const {
    return accumulateOperation<AddOp>(T(0), this->innerMultiply(other));
  }

  //dotProduct
  T dotProduct(const PointN& other) const {
    return dot(other);
  }

  //stride 
  PointN stride() const
  {
    auto pdim = (int)coords.size();
    auto ret = PointN(pdim);
    ret[0] = 1;
    for (int I = 0; I < pdim - 1; I++)
      ret[I + 1] = ret[I] * get(I);
    return ret;
  }

  //innerMultiply
  PointN innerMultiply(const PointN& other) const {
    return applyOperation<MulOp>(*this, other);
  }

  //innerDiv
  PointN innerDiv(const PointN& other) const {
    return applyOperation<DivOp>(*this, other);
  }

  //innerProduct 
  T innerProduct() const
  {
    auto pdim = getPointDim();
    if (pdim == 0) 
      return 0;

    //check overflow
#ifdef VISUS_DEBUG
    T __acc__ = 1;
    for (int I = 0; I < pdim; I++)
      if (!Utils::safe_mul(__acc__, __acc__, coords[I]))
        VisusAssert(false);
#endif

    return accumulateOperation<MulOp>(1, *this);
  }

  //innerMod
#if !SWIG

  //leftShift
  template <typename = std::enable_if_t<std::is_integral<T>::value > >
  PointN leftShift(const T & value) const {
    return applyOperation(*this, LShiftByValue(value));
  }

  //rightShift
  template <typename = std::enable_if_t<std::is_integral<T>::value > >
  PointN rightShift(const T & value) const {
    return applyOperation(*this, RShiftByValue(value));
  }

  //leftShift
  template <typename = std::enable_if_t<std::is_integral<T>::value > >
  PointN leftShift(const PointN & value) const {
    return applyOperation<LShiftOp>(*this, value);
  }

  //rightShift
  template <typename = std::enable_if_t<std::is_integral<T>::value > >
  PointN rightShift(const PointN & value) const {
    return applyOperation<RShiftOp>(*this, value);
  }

  //getLog2
  template <typename = std::enable_if_t<std::is_integral<T>::value > >
  PointN getLog2() const {
    return applyOperation<Log2Op>(*this);
  }

  template <typename = std::enable_if_t<std::is_integral<T>::value > >
  PointN innerMod(const PointN & other) const {
    return applyOperation<ModOp>(*this, other);
  }
#endif

public:

  //toPoint2
  Point2<T> toPoint2() const {
    auto coords = this->coords;
    coords.resize(2);
    return Point2<T>(coords[0], coords[1]);
  }

  //toPoint3
  Point3<T> toPoint3() const {
    auto coords = this->coords;
    coords.resize(3);
    return Point3<T>(coords[0], coords[1], coords[2]);
  }

  //toPoint4
  Point4<T> toPoint4() const {
    auto coords = this->coords;
    coords.resize(4);
    return Point4<T>(coords[0], coords[1], coords[2],coords[3]);
  }

public:

  //parseFromString
  static PointN parseFromString(String src)
  {
    std::vector<T> ret;
    std::istringstream parser(src);
    T parsed; while (parser >> parsed)
      ret.push_back(parsed);
    return PointN(ret);
  }

  //parseDims
  static PointN parseDims(String src)
  {
    auto ret = parseFromString(src);

    //backward compatible: remove unnecessary dimensions
    while (ret.getPointDim() && ret.back() == T(1))
      ret.pop_back();

    return ret;
  }

  //convert to string
  String toString(String sep = " ") const {
    auto pdim = (int)coords.size();
    std::ostringstream out;
    for (int I = 0; I < pdim; I++)
      out << (I ? sep : "") << get(I);
    return out.str();
  }

#if !SWIG
  //operator<<
  friend std::ostream& operator<<(std::ostream& out, const PointN& p) {
    out << "<" << p.toString(",") << ">";
    return out;
  }
#endif

private:

  struct NegOp  { static T compute(T a) { return -a; } };
  struct Log2Op { static T compute(T a) { return Utils::getLog2(a); } };
  struct AbsOp  { static T compute(T a) { return a >= 0 ? +a : -a; } };
  struct InvOp  { static T compute(T a) { return (T)(1.0 / a); } };

  struct AddOp { static T compute(T a, T b) { return a + b; } };
  struct SubOp { static T compute(T a, T b) { return a - b; } };
  struct MulOp { static T compute(T a, T b) { return a * b; } };
  struct DivOp { static T compute(T a, T b) { return a / b; } };
  struct ModOp { static T compute(T a, T b) { return a % b; } };

  struct MinOp { static T compute(T a, T b) { return std::min(a, b); } };
  struct MaxOp { static T compute(T a, T b) { return std::max(a, b); } };

  struct LShiftOp { static T compute(T a, T b) { return a << b; } };
  struct RShiftOp { static T compute(T a, T b) { return a >> b; } };

  struct ConditionL  { static bool isTrue(T a, T b) { return a <  b; } };
  struct ConditionLE { static bool isTrue(T a, T b) { return a <= b; } };
  struct ConditionG  { static bool isTrue(T a, T b) { return a >  b; } };
  struct ConditionGE { static bool isTrue(T a, T b) { return a >= b; } };

  struct ConditionValidNumber { static bool isTrue(T a) { return Utils::isValidNumber(a); } };

  template <typename Coeff>
  class MulByCoeff {
  public:
    Coeff value;
    MulByCoeff(Coeff value_ = Coeff(0)) : value(value_) {}
    T compute(T a) { return (T)(a * value); }
  };

  class LShiftByValue  {
  public:
    T value;
    LShiftByValue(T value_ = T(0)) : value(value_) {}
    T compute(T a) { return a << value; }
  };

  class RShiftByValue {
  public:
    T value;
    RShiftByValue(T value_ = T(0)) : value(value_) {}
    T compute(T a) { return a >> value; }
  };

  //applyOperation
  template <typename Operation>
  static PointN applyOperation(const PointN& a)
  {
    auto pdim = a.getPointDim();
    PointN ret(pdim);
    switch (pdim)
    {
      case 5: ret[4] = Operation::compute(a[4]); /*following below*/
      case 4: ret[3] = Operation::compute(a[3]); /*following below*/
      case 3: ret[2] = Operation::compute(a[2]); /*following below*/
      case 2: ret[1] = Operation::compute(a[1]); /*following below*/
      case 1: ret[0] = Operation::compute(a[0]); /*following below*/
      case 0: return ret;
      default:
        for (int I = 0; I < pdim; I++)
          ret[I] = Operation::compute(a[I]);
        return ret;
    }
  }

  //applyOperation
  template <typename Operation>
  static PointN applyOperation(const PointN& a, Operation op)
  {
    auto pdim = a.getPointDim();
    PointN ret(pdim);
    switch (pdim)
    {
    case 5: ret[4] = op.compute(a[4]); /*following below*/
    case 4: ret[3] = op.compute(a[3]); /*following below*/
    case 3: ret[2] = op.compute(a[2]); /*following below*/
    case 2: ret[1] = op.compute(a[1]); /*following below*/
    case 1: ret[0] = op.compute(a[0]); /*following below*/
    case 0: return ret;
    default:
      for (int I = 0; I < pdim; I++)
        ret[I] = op.compute(a[I]);
      return ret;
    }
  }

  //applyOperation
  template <typename Operation>
  static PointN applyOperation(const PointN& a, const PointN& b)
  {
    auto pdim = a.getPointDim();
    VisusAssert(pdim == b.getPointDim());
    PointN ret(pdim);
    switch (pdim)
    {
    case 5: ret[4] = Operation::compute(a[4], b[4]); /*following below*/
    case 4: ret[3] = Operation::compute(a[3], b[3]); /*following below*/
    case 3: ret[2] = Operation::compute(a[2], b[2]); /*following below*/
    case 2: ret[1] = Operation::compute(a[1], b[1]); /*following below*/
    case 1: ret[0] = Operation::compute(a[0], b[0]); /*following below*/
    case 0: return ret;
    default:
      for (int I = 0; I < pdim; I++)
        ret[I] = Operation::compute(a[I], b[I]);
      return ret;
    }
  }

  //checkAll
  template <typename Condition>
  static bool checkAll(const PointN& a) {
    auto pdim = a.getPointDim();
    switch (pdim)
    {
    case 5: if (!Condition::isTrue(a[4])) return false; /*following below*/
    case 4: if (!Condition::isTrue(a[3])) return false; /*following below*/
    case 3: if (!Condition::isTrue(a[2])) return false; /*following below*/
    case 2: if (!Condition::isTrue(a[1])) return false; /*following below*/
    case 1: if (!Condition::isTrue(a[0])) return false; /*following below*/
    case 0: return true;
    default:
      for (int I = 0; I < pdim; I++)
        if (!Condition::isTrue(a[I])) return false;
      return true;
    }
  }

  //checkAll
  template <typename Condition>
  static bool checkAll(const PointN& a, const PointN& b) {
    auto pdim = a.getPointDim();
    switch (pdim)
    {
    case 5: if (!Condition::isTrue(a[4], b[4])) return false; /*following below*/
    case 4: if (!Condition::isTrue(a[3], b[3])) return false; /*following below*/
    case 3: if (!Condition::isTrue(a[2], b[2])) return false; /*following below*/
    case 2: if (!Condition::isTrue(a[1], b[1])) return false; /*following below*/
    case 1: if (!Condition::isTrue(a[0], b[0])) return false; /*following below*/
    case 0: return true;
    default:
      for (int I = 0; I < pdim; I++)
        if (!Condition::isTrue(a[I], b[I])) return false;
      return true;
    }
  }

  //accumulateOperation
  template <typename AccumulateOp>
  static T accumulateOperation(T initial_value, const PointN& a)
  {
    T ret = initial_value;
    auto pdim = a.getPointDim();
    switch (pdim)
    {
    case 5: ret = AccumulateOp::compute(ret, a[0]); ret = AccumulateOp::compute(ret, a[1]); ret = AccumulateOp::compute(ret, a[2]); ret = AccumulateOp::compute(ret, a[3]); ret = AccumulateOp::compute(ret, a[4]); return ret;
    case 4: ret = AccumulateOp::compute(ret, a[0]); ret = AccumulateOp::compute(ret, a[1]); ret = AccumulateOp::compute(ret, a[2]); ret = AccumulateOp::compute(ret, a[3]); return ret;
    case 3: ret = AccumulateOp::compute(ret, a[0]); ret = AccumulateOp::compute(ret, a[1]); ret = AccumulateOp::compute(ret, a[2]); return ret;
    case 2: ret = AccumulateOp::compute(ret, a[0]); ret = AccumulateOp::compute(ret, a[1]); return ret;
    case 1: ret = AccumulateOp::compute(ret, a[0]); return ret;
    case 0: return ret;
    default:
      for (int I = 0; I < pdim; I++)
        ret = AccumulateOp::compute(ret, a[I]);
      return ret;
    }
  }

};//end class PointN

template <typename Value, typename T>
inline PointN<T> operator*(Value s, const PointN<T>& p) {
  return p * s;
}

typedef  PointN<double> PointNd;
typedef  PointN<Int64>  PointNi;


template <typename T>
inline typename PointN<T>::ForEachPoint ForEachPoint(PointN<T> from, PointN<T> to, PointN<T> step) {
  return typename PointN<T>::ForEachPoint(from, to, step);
}

template <typename T>
inline typename PointN<T>::ForEachPoint ForEachPoint(PointN<T> dims) {
  auto pdim = dims.getPointDim();
  return ForEachPoint(PointN<T>(pdim), dims, PointN<T>::one(pdim));
}

} //namespace Visus


#endif //VISUS_POINT__H
