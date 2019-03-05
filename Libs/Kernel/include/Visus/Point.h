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

  T x = T(0), y = T(0);

  //default constructor
  Point2() {
  }

  //constructor
  explicit Point2(T x_, T y_) : x(x_), y(y_) {
  }

  //constructor from string
  explicit Point2(String value) {
    std::istringstream parser(value); parser >> x >> y;
  }

  //constructor
  explicit Point2(const std::array<T, 2>& src) : x(src[0]), y(src[1]) {
  }

  //toStdArray
  std::array<T, 2> toStdArray() const {
    return { {x,y} };
  }

  //module*module
  T module2() const {
    return x*x + y*y;
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
    return Point2(x / len, y / len);
  }

  //abs
  Point2 abs() const {
    return Point2(x >= 0 ? +x : -x, y >= 0 ? +y : -y);
  }

  //inv
  Point2 inv() const {
    return Point2((T)(1.0 / x), (T)(1.0 / y));
  }

  //-a
  Point2 operator-()  const {
    return Point2(-this->x, -this->y);
  }

  //a+b
  Point2 operator+(const Point2&  b)  const {
    return Point2(this->x + b.x, this->y + b.y);
  }

  //a+=b
  Point2& operator+=(const Point2&  b) {
    this->x += b.x; this->y += b.y; return *this;
  }

  //a-b
  Point2 operator-(const Point2&  b)  const {
    return Point2(this->x - b.x, this->y - b.y);
  }

  //a-=b
  Point2& operator-=(const Point2&  b) {
    this->x -= b.x; this->y -= b.y; return *this;
  }

  //a*f
  template <typename Value>
  Point2 operator*(Value value) const {
    return Point2((T)(this->x*value), (T)(this->y*value));
  }

  //a*=f
  Point2& operator*=(T s) {
    this->x = this->x*s; this->y = this->y*s; return *this;
  }

  //a==b
  bool operator==(const Point2& b) const {
    return  x == b.x && y == b.y;
  }

  //a!=b
  bool operator!=(const Point2& b) const {
    return !(operator==(b));
  }

  //dot product
  T dot(const Point2&  b) const {
    return this->x*b.x + this->y*b.y;
  }

  //dot product
  T operator*(const Point2& b) const {
    return this->dot(b);
  }

  //access an item using an index
  T& operator[](int i) {
    VisusAssert(i >= 0 && i < 2); return i ? y : x;
  }

  //access an item using an index
  const T& operator[](int i) const {
    VisusAssert(i >= 0 && i < 2); return i ? y : x;
  }

  //test if numers are ok
  bool valid() const
  {
    return
      Utils::isValidNumber(x) &&
      Utils::isValidNumber(y);
  }

  // return index of smallest/largest value
  int biggest () const { return (x >  y) ? (0) : (1); }
  int smallest() const { return (x <= y) ? (0) : (1); }

  //convert to string
  String toString() const
  {
    std::ostringstream out;
    out << this->x << " " << this->y;
    return out.str();
  }

  //operator<<
#if !SWIG
  friend std::ostream& operator<<(std::ostream &out, const Point2& p) {
    out << "<" << p.x << "," << p.y << ">"; return out;
  }
#endif

};//end class Point2

template <typename Value, typename T>
inline Point2<T> operator*(Value value, const Point2<T> &v) {
  return v*value;
}

typedef  Point2<int   > Point2i;
typedef  Point2<float > Point2f;
typedef  Point2<double> Point2d;

template <>
inline Point2d convertTo(const Point2i& value) {
  return Point2d((double)value.x, (double)value.y);
}

////////////////////////////////////////////////////////////////////////
template <typename T>
class Point3
{
public:

  T x=T(0), y = T(0), z = T(0);

  //default constructor
  Point3() {
  }

  //constructor
  explicit Point3(T x_, T y_, T z_ = T(0)) : x(x_), y(y_), z(z_) {
  }

  //constructor
  explicit Point3(const Point2<T>& src, T z_ = T(0)) : x(src.x), y(src.y), z(z_) {
  }

  //constructor
  explicit Point3(const std::array<T, 3>& src) : x(src[0]), y(src[1]), z(src[2]) {
  }

  //constructor from string
  explicit Point3(String value) {
    std::istringstream parser(value); parser >> x >> y >> z;
  }

  //toStdArray
  std::array<T,3> toStdArray() const {
    return { {x,y,z} };
  }

  //dropZ
  Point2<T> dropZ() const {
    return Point2<T>(x, y);
  }

  //dropHomogeneousCoordinate
  Point2<T> dropHomogeneousCoordinate() const {
    return Point2<T>(x / z, y / z);
  }

  //module*module
  T module2() const {
    return x*x + y*y + z*z;
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
    return Point3((T)(x / len), (T)(y / len), (T)(z / len));
  }

  //abs
  Point3 abs() const {
    return Point3(x >= 0 ? +x : -x, y >= 0 ? +y : -y, z >= 0 ? +z : -z);
  }

  //inverse
  Point3 inv() const {
    return Point3((T)(1.0 / x), (T)(1.0 / y), (T)(1.0 / z));
  }

  //+a
  const Point3& operator+()  const {
    return *this;
  }

  //-a
  Point3 operator-()  const {
    return Point3(-this->x, -this->y, -this->z);
  }

  //a+b
  Point3 operator+(const Point3&  b)  const {
    return Point3(this->x + b.x, this->y + b.y, this->z + b.z);
  }

  //a+=b
  Point3& operator+=(const Point3&  b) {
    this->x += b.x; this->y += b.y; this->z += b.z; return *this;
  }

  //a-b
  Point3 operator-(const Point3&  b)  const {
    return Point3(this->x - b.x, this->y - b.y, this->z - b.z);
  }

  //a-=b
  Point3& operator-=(const Point3&  b) {
    this->x -= b.x; this->y -= b.y; this->z -= b.z; return *this;
  }

  //a*f
  Point3 operator*(T s) const {
    return Point3(this->x*s, this->y*s, this->z*s);
  }

  //a*=f
  template <typename Value>
  Point3& operator*=(Value value) {
    this->x = this->x*value; this->y = this->y*value; this->z = this->z*value; return *this;
  }

  //a==b
  bool operator==(const Point3& b) const {
    return  x == b.x && y == b.y && z == b.z;
  }

  //a!=b
  bool operator!=(const Point3& b) const {
    return  !(operator==(b));
  }

  //dot product
  T dot(const Point3&  b) const {
    return this->x*b.x + this->y*b.y + this->z*b.z;
  }

  //dot product
  T operator*(const Point3& b) const {
    return this->dot(b);
  }

  //access an item using an index
  T& operator[](int i) {
    VisusAssert(i >= 0 && i < 3); return (i == 0) ? (x) : (i == 1 ? y : z);
  }

  //access an item using an index
  const T& operator[](int i) const {
    VisusAssert(i >= 0 && i < 3); return (i == 0) ? (x) : (i == 1 ? y : z);
  }

  //set
  Point3& set(int index, T value) {
    (*this)[index] = value; return *this;
  }

  //cross product
  Point3 cross(const Point3& v) const {
    return Point3 (
      y * v.z - v.y * z,
      z * v.x - v.z * x,
      x * v.y - v.x * y
    );
  }

  //test if numers are ok
  bool valid() const
  {
    return
      Utils::isValidNumber(x) &&
      Utils::isValidNumber(y) &&
      Utils::isValidNumber(z);
  }

  // return index of smallest/largest value
  int biggest () const { return (x >  y) ? (x >  z ? 0 : 2) : (y >  z ? 1 : 2); }
  int smallest() const { return (x <= y) ? (x <= z ? 0 : 2) : (y <= z ? 1 : 2); }

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
    out << this->x << " " << this->y << " " << this->z;
    return out.str();
  }


#if !SWIG
  //operator<<
  friend std::ostream& operator<<(std::ostream &out,const Point3& p) {
    out << "<" << p.x << "," << p.y << "," << p.z << ">"; return out;
  }
#endif

};//end class Point3

template <typename Value, typename T>
inline Point3<T> operator*(Value value, const Point3<T> &v) {
  return v*value;
}

typedef  Point3<int  >  Point3i;
typedef  Point3<Int64>  Point3i64;
typedef  Point3<float>  Point3f;
typedef  Point3<double> Point3d;

template <>
inline Point3d convertTo(const Point3i& value) {
  return Point3d((double)value.x, (double)value.y, (double)value.z);
}

template <>
inline Point3d convertTo(const Point3i64& value) {
  return Point3d((double)value.x, (double)value.y, (double)value.z);
}

///////////////////////////////////////////////////////////////////////////
template <typename T>
class Point4
{
public:

  T x= T(0), y = T(0), z = T(0), w = T(0);

  //default constructor
  Point4() {
  }

  //constructor
  explicit Point4(T _x, T _y, T _z = 0, T _w = 0)
    : x(_x), y(_y), z(_z), w(_w) {
  }

  //constructor
  explicit Point4(const Point3<T>& src, T _w = 0)
    : x(src.x), y(src.y), z(src.z), w(_w) {
  }

  //constructor
  explicit Point4(const std::array<T, 4>& src) : x(src[0]), y(src[1]), z(src[2]), w(src[3]) {
  }

  //constructor from string
  explicit Point4(String value) {
    std::istringstream parser(value); parser >> x >> y >> z >> w;
  }

  //toStdArray
  std::array<T, 4> toStdArray() const {
    return { {x,y,z,w} };
  }

  //dropW
  Point3<T> dropW() const {
    return Point3<T>(x, y, z);
  }

  //dropHomogeneousCoordinate
  Point3<T> dropHomogeneousCoordinate() const
  {
    T W = this->w;
    if (!W) W = 1;
    return Point3<T>(x / W, y / W, z / W);
  }

  //module2
  T module2() const {
    return x*x + y*y + z*z + w*w;
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
    return Point4(x * vs, y * vs, z * vs, w * vs);
  }

  //abs
  Point4 abs() const {
    return Point4(x >= 0 ? +x : -x, y >= 0 ? +y : -y, z >= 0 ? +z : -z, w >= 0 ? +w : -w);
  }

  //inverse
  Point4 inv() const {
    return Point4((T)(1.0 / x), T(1.0 / y), (T)(1.0 / z), (T)(1.0 / w));
  }

  //operator-
  Point4 operator-()  const {
    return Point4(-x, -y, -z, -w);
  }

  //a+b
  Point4 operator+(const Point4& b)  const {
    return Point4(x + b.x, y + b.y, z + b.z, w + b.w);
  }

  //a+=b
  Point4& operator+=(const Point4&  b) {
    x += b.x; y += b.y; z += b.z; w += b.w; return *this;
  }

  //a-b
  Point4 operator-(const Point4&  b)  const{
    return Point4(x - b.x, y - b.y, z - b.z, w - b.w);
  }

  //a-=b
  Point4& operator-=(const Point4&  b) {
    x -= b.x; y -= b.y; z -= b.z; w -= b.w; return *this;
  }

  //a*f
  template <typename Value>
  Point4 operator*(Value value) const {
    return Point4(x*value, y*value, z*value, w*value);
  }

  //a*=f
  template <typename Value>
  Point4& operator*=(Value value) {
    return (*this=*this * value);
  }

  //a==b
  bool operator==(const Point4& b) const {
    return  x == b.x && y == b.y && z == b.z && w == b.w;
  }

  //a!=b
  bool operator!=(const Point4& b) const {
    return  !(operator==(b));
  }

  //dot product
  T dot(const Point4&  b) const {
    return x*b.x + y*b.y + z*b.z + w*b.w;
  }

  //dot product
  T operator*(const Point4& b) const  {
    return this->dot(b);
  }

  //access an item using an index
  T& operator[](int i) {
    VisusAssert(i >= 0 && i < 4); return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
  }

  //access an item using an index
  const T& operator[](int i) const {
    VisusAssert(i >= 0 && i < 4); return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
  }

  //test if numers are ok
  bool valid() const
  {
    return
      Utils::isValidNumber(x) &&
      Utils::isValidNumber(y) &&
      Utils::isValidNumber(z) &&
      Utils::isValidNumber(w);
  }

public:

  //convert to string
  String toString() const
  {
    std::ostringstream out;
    out << this->x << " " << this->y << " " << this->z << " " << this->w;
    return out.str();
  }


#if !SWIG
  //operator<<
  friend std::ostream& operator<<(std::ostream &out,const Point4& p)  {
    out << "<" << p.x << "," << p.y << "," << p.z << "," << p.w << ">"; return out;
  }
#endif

};//end class Point4

template <typename Value, typename T>
inline Point4<T> operator*(Value value, const Point4<T> &v) {
  return v*value;
}

typedef Point4<int   > Point4i;
typedef Point4<float > Point4f;
typedef Point4<double> Point4d;


template <>
inline Point4d convertTo(const Point4i& value) {
  return Point4d((double)value.x, (double)value.y, (double)value.z, (double)value.w);
}


#ifndef VISUS_NDPOINT_DIM
  #define VISUS_NDPOINT_DIM 5
#endif

//////////////////////////////////////////////////////////////
template <typename T>
class PointN
{
  int pdim = 0;

  std::array<T, VISUS_NDPOINT_DIM> coords = std::array<T, VISUS_NDPOINT_DIM>({ {0,0,0,0,0} });

public:

  typedef T coord_t;

  struct Compare
  {
    inline bool operator()(const PointN& a, const  PointN& b) const {
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
      VisusAssert(pdim == to  .getPointDim());
      VisusAssert(pdim == step.getPointDim());
      VisusAssert(pdim == pos .getPointDim());

      if (!pdim)
        return;

      for (int D = 0; D < pdim; D++) {
        if (!(pos[D] >= from[D] && pos[D] < to[D])) 
          return;
      }

      this->bEnd = false;
    }

    //end
    inline bool end() const {
      return bEnd;
    }

    //next
    inline void next()
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


  //default constructor
  PointN() {
  }

  //constructor
  explicit PointN(int pdim_, T x, T y, T z, T u, T v)
  : pdim(pdim_), coords({ {x,y,z,u,v} }) {
  }

  //constructor
  explicit PointN(int pdim) : PointN(pdim, 0, 0, 0, 0, 0) {
  }


  //constructor
  explicit PointN(T x, T y) : PointN(2, x, y, 0, 0, 0) {
  }

  //constructor
  explicit PointN(T x, T y, T z) : PointN(3, x, y, z, 0, 0) {
  }

  //constructor
  explicit PointN(T x, T y, T z, T u) : PointN(4, x, y, z, u, 0) {
  }

  //constructor
  explicit PointN(T x, T y, T z, T u, T v) : PointN(5, x, y, z, u, v) {
  }

  //constructor
  explicit PointN(Point3<T> p) : PointN(p.x, p.y, p.z) {
  }

  //constructor
  explicit PointN(const std::vector<T>& src) : 
    PointN((int)src.size(),
      src.size() >= 1 ? src[0] : 0,
      src.size() >= 2 ? src[1] : 0,
      src.size() >= 3 ? src[2] : 0,
      src.size() >= 4 ? src[3] : 0,
      src.size() >= 5 ? src[4] : 0) {
  }

  //getPointDim
  int getPointDim() const {
    return pdim;
  }

  //setPointDim
  void setPointDim(int value) {
    pdim = value;
  }

  //withPointDim
  PointN withPointDim(int new_pdim,T embed_value) const {
    return PointN(new_pdim, 
      pdim >= 1 ? get(0) : embed_value, 
      pdim >= 2 ? get(1) : embed_value,
      pdim >= 3 ? get(2) : embed_value,
      pdim >= 4 ? get(3) : embed_value,
      pdim >= 5 ? get(4) : embed_value);
  }

  //constructor
  static inline PointN one(int pdim) {
    return PointN(pdim,1, 1, 1, 1, 1);
  }

  //one
  static inline PointN one(T x, T y) {
    auto ret = PointN::one(2);
    ret[0] = x;
    ret[1] = y;
    return ret;
  }

  //one
  static inline PointN one(T x, T y, T z) {
    auto ret = PointN::one(3);
    ret[0] = x;
    ret[1] = y;
    ret[2] = z;
    return ret;
  }

  //toVector
  template <typename Cast>
  std::vector<Cast> toVector() const {
    std::vector<Cast> ret(pdim);
    for (int I = 0; I < pdim; I++)
      ret[I] = (Cast)(get(I));
    return ret;
  }

  //toArray
  std::vector<T> toVector() const {
    return std::vector<T>(coords.begin(),coords.begin()+pdim);
  }

  //test if numers are ok
  bool valid() const {
    return 
      (pdim<1 || (Utils::isValidNumber(get(0)) &&
      (pdim<2 || (Utils::isValidNumber(get(1)) &&
      (pdim<3 || (Utils::isValidNumber(get(2)) &&
      (pdim<4 || (Utils::isValidNumber(get(3)) &&
      (pdim<5 || (Utils::isValidNumber(get(4))))))))))));
  }

  //withValueOnAxis
  PointN withValueOnAxis(int axis, T value) const {
    PointN ret = *this; 
    ret[axis] = value; 
    return ret;
  }

  PointN withX(T value) { return withValueOnAxis(0, value); }
  PointN withY(T value) { return withValueOnAxis(1, value); }
  PointN withZ(T value) { return withValueOnAxis(2, value); }

  //get
  inline T& get(int i) {
    VisusAssert(i >= 0 && i < 5);
    return coords[i];
  }

  //const
  inline const T& get(int i) const {
    VisusAssert(i >= 0 && i < 5);
    return coords[i];
  }

  //set
  inline void set(int i, T value) {
    VisusAssert(i >= 0 && i < 5);
    coords[i]=value;
  }

  //operator[]
  const T& operator[](int i) const {
    VisusAssert(i >= 0 && i < 5);
    return coords[i];
  }

  //operator[]
  inline T& operator[](int i) {
    VisusAssert(i >= 0 && i < 5);
    return coords[i];
  }

  //operator-
  PointN operator-()  const {
    return PointN(pdim,-get(0),-get(1),-get(2),-get(3),-get(4));
  }

  //a+b
  PointN operator+(const PointN& other)  const {
    VisusAssert(pdim == other.pdim);
    return PointN(pdim,get(0) + other[0], get(1) + other[1], get(2) + other[2], get(3) + other[3], get(4) + other[4]);
  }

  //a-b
  PointN operator-(const PointN&  other)  const {
    VisusAssert(pdim == other.pdim);
    return PointN(pdim, get(0) - other[0], get(1) - other[1], get(2) - other[2], get(3) - other[3], get(4) - other[4]);
  }

  //a*s
  template <typename Value>
  PointN operator*(Value value) const {
    return PointN(pdim, (T)(get(0) * value), (T)(get(1) * value), (T)(get(2) * value), (T)(get(3) * value), (T)(get(4) * value));
  }

  //a+=b
  PointN& operator+=(const PointN&  other) {
    return ((*this) = (*this) + other);
  }

  //a-=b
  PointN& operator-=(const PointN&  other) {
    return ((*this) = (*this) - other);
  }

  //a*=s
  PointN& operator*=(T s) {
    return ((*this) = (*this) * s);
  }

  //a==b
  bool operator==(const PointN& other) const {
    return get(0) == other[0] && get(1) == other[1] && get(2) == other[2] && get(3) == other[3] && get(4) == other[4];
  }

  //a!=b
  bool operator!=(const PointN& other) const {
    return !(operator==(other));
  }

  //min
  static PointN min(const PointN& a, const PointN& b) { 
    VisusAssert(a.pdim == b.pdim);
    return PointN(a.pdim, std::min(a[0] , b[0]), std::min(a[1], b[1]), std::min(a[2], b[2]), std::min(a[3], b[3]), std::min(a[4], b[4]));
  }
  
  //max
  static PointN max(const PointN& a, const PointN& b) { 
    VisusAssert(a.pdim == b.pdim);
    return PointN(a.pdim,std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2]), std::max(a[3], b[3]), std::max(a[4], b[4]));
  }

  //minsize
  T minsize() const {
    return Utils::min(toVector());
  }

  //maxsize
  T maxsize() const {
    return Utils::max(toVector());
  }

  //operator (<,<=,>,>=)
  bool operator< (const PointN& b) const { const PointN& a = *this; VisusAssert(a.pdim == b.pdim); return pdim<=0 || (a[0] <  b[0] && (pdim<=1 || (a[1] <  b[1] && (pdim<=2 || (a[2] <  b[2] && (pdim<=3 || (a[3] <  b[3] && (pdim<=4 || (a[4] <  b[4]))))))))); }
  bool operator<=(const PointN& b) const { const PointN& a = *this; VisusAssert(a.pdim == b.pdim); return pdim<=0 || (a[0] <= b[0] && (pdim<=1 || (a[1] <= b[1] && (pdim<=2 || (a[2] <= b[2] && (pdim<=3 || (a[3] <= b[3] && (pdim<=4 || (a[4] <= b[4]))))))))); }
  bool operator> (const PointN& b) const { const PointN& a = *this; VisusAssert(a.pdim == b.pdim); return pdim<=0 || (a[0] >  b[0] && (pdim<=1 || (a[1] >  b[1] && (pdim<=2 || (a[2] >  b[2] && (pdim<=3 || (a[3] >  b[3] && (pdim<=4 || (a[4] >  b[4]))))))))); }
  bool operator>=(const PointN& b) const { const PointN& a = *this; VisusAssert(a.pdim == b.pdim); return pdim<=0 || (a[0] >= b[0] && (pdim<=1 || (a[1] >= b[1] && (pdim<=2 || (a[2] >= b[2] && (pdim<=3 || (a[3] >= b[3] && (pdim<=4 || (a[4] >= b[4]))))))))); }

  static bool less        (const PointN& a, const PointN& b) { return a <  b;}
  static bool lessEqual   (const PointN& a, const PointN& b) { return a <= b; }
  static bool greater     (const PointN& a, const PointN& b) { return a >  b; }
  static bool greaterEqual(const PointN& a, const PointN& b) { return a >= b; }

public:

  //dot product
  T dot(const PointN& other) const {
    VisusAssert(pdim == other.pdim);
    return 
      (pdim >= 1 ? (get(0) * other[0]) : 0) +
      (pdim >= 2 ? (get(1) * other[1]) : 0) +
      (pdim >= 3 ? (get(2) * other[2]) : 0) +
      (pdim >= 4 ? (get(3) * other[3]) : 0) +
      (pdim >= 5 ? (get(4) * other[4]) : 0);
  }

  //dotProduct
  T dotProduct(const PointN& other) const {
    return dot(other);
  }

  //leftShift
  template <typename Value>
  inline PointN leftShift(const Value& value) const {
    return PointN(pdim,get(0) << value, get(1) << value, get(2) << value, get(3) << value, get(4) << value);
  }

  //leftShift
  template <typename Other>
  inline PointN leftShift(const PointN<Other>& value) const {
    VisusAssert(pdim==value.pdim);
    return PointN(pdim,get(0) << value[0], get(1) << value[1], get(2) << value[2], get(3) << value[3], get(4) << value[4]);
  }

  //rightShift
  template <typename Value>
  inline PointN rightShift(const Value& value) const {
    return PointN(pdim,get(0) >> value, get(1) >> value, get(2) >> value, get(3) >> value, get(4) >> value);
  }

  //rightShift
  template <typename Other>
  inline PointN rightShift(const PointN<Other>& value) const {
    VisusAssert(pdim == value.pdim);
    return PointN(pdim,get(0) >> value[0], get(1) >> value[1], get(2) >> value[2], get(3) >> value[3], get(4) >> value[4]);
  }

  //getLog2
  inline PointN getLog2() const {
    return PointN(pdim, Utils::getLog2(get(0)), Utils::getLog2(get(1)), Utils::getLog2(get(2)), Utils::getLog2(get(3)), Utils::getLog2(get(4)));
  }

  //stride 
  inline PointN stride() const
  {
    VisusAssert(!overflow());
    PointN ret=one(pdim); 
    ret[0] = 1; 
    ret[1] = ret[0]*(pdim >= 1 ? this->get(0) : 1); 
    ret[2] = ret[1]*(pdim >= 2 ? this->get(1) : 1);
    ret[3] = ret[2]*(pdim >= 3 ? this->get(2) : 1);
    ret[4] = ret[3]*(pdim >= 4 ? this->get(3) : 1);
    return ret;
  }

  //innerMultiply
  inline PointN innerMultiply(const PointN& other) const {
    VisusAssert(pdim == other.pdim);
    return PointN(pdim,this->get(0)*other[0], this->get(1)*other[1], this->get(2)*other[2], this->get(3)*other[3], this->get(4)*other[4]);
  }

  //innerDiv
  inline PointN innerDiv(const PointN& other) const {
    VisusAssert(pdim == other.pdim);
    return PointN(pdim, this->get(0)/other[0], this->get(1)/other[1], this->get(2)/other[2], this->get(3)/other[3], this->get(4)/other[4]);
  }

  //innerProduct (must return -1 in case of overflow)
  T innerProduct() const
  {
    T ret = 1;
    if (pdim >= 1 && !Utils::notoverflow_mul(ret, ret, get(0))) return -1;
    if (pdim >= 2 && !Utils::notoverflow_mul(ret, ret, get(1))) return -1;
    if (pdim >= 3 && !Utils::notoverflow_mul(ret, ret, get(2))) return -1;
    if (pdim >= 4 && !Utils::notoverflow_mul(ret, ret, get(3))) return -1;
    if (pdim >= 5 && !Utils::notoverflow_mul(ret, ret, get(4))) return -1;
    return ret;
  }

  //overflow (the 64 bit limit!)
  inline bool overflow() const {
    return innerProduct() < 0;
  }

public:

  //toPoint3
  Point3<T> toPoint3() const {
    return Point3<T>(get(0), get(1), get(2));
  }

  //toPoint3i
  inline Point3i toPoint3i() const {
    return Point3i((int)get(0), (int)get(1), (int)get(2));
  }

  //toPoint3d
  inline Point3d toPoint3d() const {
    return Point3d((double)get(0), (double)get(1), (double)get(2));
  }

public:

  //parseFromString
  static inline PointN parseFromString(String src)
  {
    PointN ret;
    std::istringstream parser(src);
    T value;
    while (parser >> value)
      ret[ret.pdim++] = value;
    return ret;
  }

  //parseDims
  static inline PointN parseDims(String src) 
  {
    PointN ret(0, 1, 1, 1, 1, 1);
    std::istringstream parser(src);
    T value;
    while (parser >> value)
      ret[ret.pdim++] = value;

    //backward compatible: remove unnecessary dimensions
    while (ret.pdim && ret[ret.pdim - 1] == 1)
      ret.pdim--;

    return ret;
  }

  //convert to string
  inline String toString(String sep=" ") const {
    std::ostringstream out;
    if (pdim >= 1) out <<        get(0);
    if (pdim >= 2) out << sep << get(1);
    if (pdim >= 3) out << sep << get(2);
    if (pdim >= 4) out << sep << get(3);
    if (pdim >= 5) out << sep << get(4);
    return out.str();
  }

  #if !SWIG
  //operator<<
  friend std::ostream& operator<<(std::ostream &out,const PointN& p) {
    out << "<" << p.tpString(",") << ">";
    return out;
  }
  #endif

};//end class PointN

template <typename Value,typename T>
inline PointN<T> operator*(Value s, const PointN<T> &p) {
  return p * s;
}



template <typename T>
inline typename PointN<T>::ForEachPoint ForEachPoint(PointN<T> from, PointN<T> to, PointN<T> step) {
  return typename PointN<T>::ForEachPoint(from, to, step);
}

template <typename T>
inline typename PointN<T>::ForEachPoint ForEachPoint(PointN<T> dims) {
  auto pdim = dims.getPointDim();
  return ForEachPoint(PointN<T>(pdim), dims, PointN<T>::one(pdim));
}


typedef PointN<double> PointNd;
typedef PointN<Int64>  PointNi;

//backward compatible
typedef PointNi NdPoint;

template <>
inline PointNd convertTo(const PointNi& value) {
  return PointNd(value.toVector<PointNd::coord_t>());
}

template <>
inline PointNi convertTo(const PointNd& value) {
  return PointNi(value.toVector<PointNi::coord_t>());
}


} //namespace Visus


#endif //VISUS_POINT__H
