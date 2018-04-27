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

#ifndef VISUS_QUATERNION_H
#define VISUS_QUATERNION_H

#include <Visus/Kernel.h>
#include <Visus/Point.h>

namespace Visus {

//////////////////////////////////////////////////////////////
template <typename T>
class Quaternion4
{
public:

  VISUS_CLASS(Quaternion4)

  //identity
  T w = T(1), x = T(0), y = T(0), z = T(0);

  //default constructor
  Quaternion4() {
  }

  //constructor
  explicit Quaternion4(T w, T x, T y, T z) 
  {
    //isNull?
    if (w == T(0) && x == T(0) && y == T(0) && z == T(0))
    {
      this->w = this->x = this->y = this->z = T(0);
      return;
    }

    T mod2 = w * w + x * x + y * y + z * z; VisusAssert(mod2 > T(0));
    T vs = (mod2==T(1)) ? T(1) : (T(1) / sqrt(mod2));
    this->w = w * vs;
    this->x = x * vs;
    this->y = y * vs;
    this->z = z * vs;
  }

  //constructor
  explicit Quaternion4(Point3<T> axis, T angle)
  {
    if (axis.x==T(0) && axis.y==T(0) && axis.z==T(0))
    {
      this->w = this->x = this->y = this->z = T(0);
      return;
    }

    axis = axis.normalized();

    T half_angle = T(0.5)*angle;
    T c = cos(half_angle);
    T s = sin(half_angle);
    
    this->w = c;
    this->x = s * axis.x;
    this->y = s * axis.y;
    this->z = s * axis.z;
  }

  //constructor from string
  explicit Quaternion4(String value) {
    std::istringstream parser(value);
    T w, x, y, z;
    parser >> w >> x >> y >> z;
    *this = Quaternion4(w, x, y, z);
  }

  //isNull
  bool isNull() const {
    return w == T(0) && x == T(0) && y == T(0) && z == T(0);
  }

  //isIdentity
  bool isIdentity() const {
    return w == T(1) && x == T(0) && y == T(0) && z == T(0);
  }

  //identity
  static Quaternion4 identity() {
    return Quaternion4();
  }

  //null
  static Quaternion4 null() {
    return Quaternion4(T(0), T(0), T(0), T(0));
  }

  //convert to string
  String toString() const {
    std::ostringstream out;
    out << this->w << " " << this->x << " " << this->y << " " << this->z;
    return out.str();
  }

  //return the main axis of rotation
  Point3<T> getAxis() const {
    return x!=T(0) || y!=T(0) || z!=T(0) ? Point3<T>(x, y, z).normalized() : Point3<T>(0,0,1);
  }

  //return the angle of rotation
   T getAngle() const {
     return T(2)*acos(Utils::clamp(w, -T(1), +T(1)));
  }

  //q1*q2
   Quaternion4 operator*(const Quaternion4& q2) const
  {
    return Quaternion4(
      w * q2.w - x * q2.x - y * q2.y - z * q2.z,
      w * q2.x + x * q2.w + y * q2.z - z * q2.y,
      w * q2.y + y * q2.w + z * q2.x - x * q2.z,
      w * q2.z + z * q2.w + x * q2.y - y * q2.x);
  }

  //operator==
  bool operator==(const Quaternion4& other) const {
    return w == other.w && x == other.x && y == other.y && z == other.z;
  }

  //operator!=
  bool operator!=(const Quaternion4& other) const {
    return !(operator==(other));
  }

  //q1*=q2
   Quaternion4& operator*=(const Quaternion4& q2) {
    (*this) = (*this)*q2; return *this;
  }

  //toEulerAngles (https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles)
  static Quaternion4 fromEulerAngles(T roll, T pitch, T yaw)
  {
	  T t0 = cos(yaw   * T(0.5));
	  T t1 = sin(yaw   * T(0.5));
	  T t2 = cos(roll  * T(0.5));
	  T t3 = sin(roll  * T(0.5));
	  T t4 = cos(pitch * T(0.5));
	  T t5 = sin(pitch * T(0.5));

    return Quaternion4(
      t0 * t2 * t4 + t1 * t3 * t5,
      t0 * t3 * t4 - t1 * t2 * t5,
      t0 * t2 * t5 + t1 * t3 * t4,
      t1 * t2 * t4 - t0 * t3 * t5);
  }

  //toEulerAngles (https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles)
  Point3<T> toEulerAngles() const 
  {
    T ysqr = y * y;

	  // roll (x-axis rotation)
	  T t0 = +T(2) * (w * x + y * z);
	  T t1 = +T(1) - T(2) * (x * x + ysqr);
	  T roll = atan2(t0, t1);

	  // pitch (y-axis rotation)
	  T t2 = +T(2) * (w * y - z * x);
	  t2 = t2 >  T(1) ? T(1) : t2;
	  t2 = t2 < -T(1) ? -T(1) : t2;
	  T pitch = asin(t2);

	  // yaw (z-axis rotation)
	  T t3 = +T(2) * (w * z + x * y);
	  T t4 = +T(1) - T(2) * (ysqr + z * z);  
	  T yaw = atan2(t3, t4);

    return Point3<T>(roll,pitch,yaw);
  }

  //conjugate
  Quaternion4 conjugate() const {
    return Quaternion4(w, -x, -y, -z);
  }

  //operator*
  Point3<T> operator*(const Point3<T>& p) const 
  {
    const T t2 = +this->w * this->x;
    const T t3 = +this->w * this->y;
    const T t4 = +this->w * this->z;
    const T t5 = -this->x * this->x;
    const T t6 = +this->x * this->y;
    const T t7 = +this->x * this->z;
    const T t8 = -this->y * this->y;
    const T t9 = +this->y * this->z;
    const T t1 = -this->z * this->z;

    T x = T(2) * ((t8 + t1) * p.x + (t6 - t4) * p.y + (t3 + t7) * p.z) + p.x;
    T y = T(2) * ((t4 + t6) * p.x + (t5 + t1) * p.y + (t9 - t2) * p.z) + p.y;
    T z = T(2) * ((t7 - t3) * p.x + (t2 + t9) * p.y + (t5 + t8) * p.z) + p.z;

    return Point3<T>(x, y, z);
  }

}; //end class Quaternion4

typedef Quaternion4<double> Quaternion4d;

} //namespace Visus

#endif //VISUS_QUATERNION_H

