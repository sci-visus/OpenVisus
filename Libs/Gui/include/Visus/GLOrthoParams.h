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

#ifndef VISUS_GL_ORTHO_PARAMS_H
#define VISUS_GL_ORTHO_PARAMS_H

#include <Visus/Gui.h>
#include <Visus/Rectangle.h>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLOrthoParams 
{
public:

  VISUS_CLASS(GLOrthoParams)

  double left,right,bottom,top,zNear,zFar;

  //default constructor
  GLOrthoParams(double left_=0,double right_=0,double bottom_=0,double top_=0,double zNear_=0,double zFar_=0) 
    : left(left_),right(right_),bottom(bottom_),top(top_),zNear(zNear_),zFar(zFar_) {
  }

  //fromString
  static GLOrthoParams fromString(String s)
  {
    GLOrthoParams ret;
    std::istringstream parser(s);
    parser >> ret.left >> ret.right >> ret.bottom >> ret.top >> ret.zNear >> ret.zFar;
    return ret;
  }

  //fromCenterAndSize
  static GLOrthoParams fromCenterAndSize(Point3d center, Point3d size) {
    return GLOrthoParams(
      center.x - 0.5 * size.x, 
      center.x + 0.5 * size.x,
      center.y - 0.5 * size.y,
      center.y + 0.5 * size.y,
      center.z - 0.5 * size.z,
      center.z + 0.5 * size.z);
  }

  //operator*
  GLOrthoParams operator*(double value) {
    return GLOrthoParams(value * left, value * right, value * bottom, value * top, value * zNear, value * zFar);
  }

  //operator+
  GLOrthoParams operator+(const GLOrthoParams other) {
    return GLOrthoParams(left + other.left, right + other.right, bottom + other.bottom, top + other.top, zNear + other.zNear, zFar + other.zFar);
  }

  //operator+
  GLOrthoParams operator-(const GLOrthoParams other) {
    return GLOrthoParams(left - other.left, right - other.right, bottom - other.bottom, top - other.top, zNear - other.zNear, zFar - other.zFar);
  }

  //toVector
  std::vector<double> toVector() const {
    return std::vector<double>({ left,right,bottom,top,zNear,zFar });
  }

  //operator==
  bool operator==(const GLOrthoParams& other) const
  {
    return 
      left==other.left &&
      right==other.right && 
      bottom==other.bottom &&
      top==other.top &&
      zNear==other.zNear &&
      zFar==other.zFar;
  }

  //operator!=
  bool operator!=(const GLOrthoParams& other) const {
    return !(operator==(other));
  }

  //getWidth
  inline double getWidth() const {
    return fabs(right - left);
  }

  //getHeight
  inline double getHeight() const {
    return fabs(top - bottom);
  }

  //getDepth
  inline double getDepth() const {
    return fabs(zFar - zNear);
  }

  //getSize
  inline Point3d getSize() const {
    return Point3d(getWidth(), getHeight(), getDepth());
  }

  //getCenter
  inline Point3d getCenter() const {
    return 0.5 * Point3d(left + right, bottom + top, zNear + zFar);
  }

  //translated
  GLOrthoParams translated(const Point3d& vt) const
  {
    return GLOrthoParams(
      this->left + vt[0], this->right + vt[0],
      this->bottom + vt[1], this->top + vt[1],
      this->zNear + vt[2], this->zFar + vt[2]);
  }

  //translated
  GLOrthoParams translated(const Point2d& vt) const {
    return translated(Point3d(vt, 0));
  }

  //scaled
  GLOrthoParams scaled(const Point3d& vs) const
  {
    return GLOrthoParams(
      vs[0] * left, vs[0] * right,
      vs[1] * bottom, vs[1] * top,
      vs[2] * zNear, vs[2] * zFar);
  }

  //translate
  GLOrthoParams scaledAroundCenter(const Point3d& vs, const Point3d& center) const {
    return translated(-center).scaled(vs).translated(+center);
  }


  //withAspectRatio
  GLOrthoParams withAspectRatio(double ratio) const
  {
    auto dx = right - left; auto cx = (left + right) * 0.5;
    auto dy = top - bottom; auto cy = (bottom + top) * 0.5;
    auto dz = zFar - zNear; auto cz = (zFar + zNear) * 0.5;

    if ((dx / dy) <= ratio)
      dx = dy * (ratio);
    else
      dy = dx * (1.0 / ratio);

    return GLOrthoParams(
      cx - 0.5 * dx, cx + 0.5 * dx,
      cy - 0.5 * dy, cy + 0.5 * dy,
      cz - 0.5 * dz, cz + 0.5 * dz);
  }

  //split
  GLOrthoParams split(Rectangle2d r) const {
    return GLOrthoParams(
      this->left + r.p1().x * (this->right - this->left),
      this->left + r.p2().x * (this->right - this->left),
      this->bottom + r.p1().y * (this->top - this->bottom),
      this->bottom + r.p2().y * (this->top - this->bottom),
      this->zNear,
      this->zFar);
  }

  //toString
  String toString() const {
    return cstring(left, right, bottom, top, zNear, zFar);
  }

public:

  //write
  void write(Archive& ar) const;

  //read
  void read(Archive& ar) ;

};



} //namespace Visus

#endif //VISUS_GL_ORTHO_PARAMS_H

