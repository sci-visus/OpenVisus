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

#include <Visus/GLOrthoParams.h>

namespace Visus {


////////////////////////////////////////////////////////////////////////
GLOrthoParams GLOrthoParams::translated(const Point3d& vt) const
{
  return GLOrthoParams(
    this->left   + vt[0], this->right + vt[0], 
    this->bottom + vt[1], this->top   + vt[1], 
    this->zNear  + vt[2], this->zFar  + vt[2]);
}

////////////////////////////////////////////////////////////////////////
GLOrthoParams GLOrthoParams::scaled(const Point3d& vs) const
{
  return GLOrthoParams(
    vs[0] * left  , vs[0] * right, 
    vs[1] * bottom, vs[1] * top, 
    vs[2] * zNear , vs[2] * zFar);
}


////////////////////////////////////////////////////////////////////////
Matrix GLOrthoParams::getProjectionMatrix(bool bUseOrthoProjection) const
{
  return bUseOrthoProjection ?
    Matrix::ortho(left, right, bottom, top, zNear, zFar) :
    Matrix::frustum(left, right, bottom, top, zNear, zFar);
}

////////////////////////////////////////////////////////////////////////
GLOrthoParams GLOrthoParams::withAspectRatio(double ratio) const
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

////////////////////////////////////////////////////////////////////////
GLOrthoParams GLOrthoParams::split(const Rectangle2d& S) const
{
  GLOrthoParams ret;
  double W = this->right - this->left;
  double H = this->top - this->bottom;
  ret.left = this->left + S.x     * W;
  ret.right = ret.left + S.width * W;
  ret.bottom = this->bottom + S.y     * H;
  ret.top = ret.bottom + S.height* H;
  ret.zNear = this->zNear;
  ret.zFar = this->zFar;
  return ret;
}



////////////////////////////////////////////////////////////////////////
void GLOrthoParams::writeTo(StringTree& out) const
{
  out.writeString("left", cstring(left));
  out.writeString("right", cstring(right));
  out.writeString("bottom", cstring(bottom));
  out.writeString("top", cstring(top));
  out.writeString("zNear", cstring(zNear));
  out.writeString("zFar", cstring(zFar));
}

////////////////////////////////////////////////////////////////////////
void GLOrthoParams::readFrom(StringTree& in) 
{
  left   = cdouble(in.readString("left"));
  right  = cdouble(in.readString("right"));
  bottom = cdouble(in.readString("bottom"));
  top    = cdouble(in.readString("top"));
  zNear  = cdouble(in.readString("zNear"));
  zFar   = cdouble(in.readString("zFar"));
}


} //namespace Visus