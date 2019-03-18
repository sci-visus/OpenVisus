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
  inline GLOrthoParams(double left_=0,double right_=0,double bottom_=0,double top_=0,double zNear_=0,double zFar_=0) 
    : left(left_),right(right_),bottom(bottom_),top(top_),zNear(zNear_),zFar(zFar_)
  {}

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
    return 0.5*Point3d(left + right, bottom + top, zNear + zFar);
  }

  //translate
  void translate(const Point3d& vt);

  //translate
  void scaleAroundCenter(const Point3d& vs, const Point3d& center);

  //getProjectionMatrix
  Matrix getProjectionMatrix(bool bUseOrthoProjection = true) const;

  //fixAspectRatio
  void fixAspectRatio(double ratio);

  //split (rect in in the range [0,1])
  GLOrthoParams split(const Rectangle2d& S) const;

  //fixAspectRatio
  void fixAspectRatio(const Viewport& old_value, const Viewport& new_value);

  //toString
  String toString() const {
    std::ostringstream out;
    out << left << " " << right << " " << top << " " << bottom << " " << zNear << " " << zFar;
    return out.str();
  }

public:

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) ;

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream) ;

};



} //namespace Visus

#endif //VISUS_GL_ORTHO_PARAMS_H

