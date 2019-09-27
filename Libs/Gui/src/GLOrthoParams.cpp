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
void GLOrthoParams::translate(const Point3d& vt)
{
  left   += vt[0]; right += vt[0];
  bottom += vt[1]; top   += vt[1];
  zNear   += vt[2]; zFar += vt[2];
}

////////////////////////////////////////////////////////////////////////
void GLOrthoParams::scaleAroundCenter(const Point3d& vs, const Point3d& center)
{
  left   = center[0] + vs[0] * (left   - center[0]);
  right  = center[0] + vs[0] * (right  - center[0]);
  bottom = center[1] + vs[1] * (bottom - center[1]);
  top    = center[1] + vs[1] * (top    - center[1]);
  zNear  = center[2] + vs[2] * (zNear  - center[2]);
  zFar   = center[2] + vs[2] * (zFar   - center[2]);
}


////////////////////////////////////////////////////////////////////////
Matrix GLOrthoParams::getProjectionMatrix(bool bUseOrthoProjection) const
{
  return bUseOrthoProjection ?
    Matrix::ortho(left, right, bottom, top, zNear, zFar) :
    Matrix::frustum(left, right, bottom, top, zNear, zFar);
}

////////////////////////////////////////////////////////////////////////
void GLOrthoParams::fixAspectRatio(double ratio)
{
  if (ratio <= 0) return;
  Point3d center = getCenter();
  Point3d size = getSize();
  if (!size[0] || !size[1] || !size[2]) return;
  if (size[0] / size[1] <= ratio) size[0] = size[1] * ratio;
  else                          size[1] = size[0] / ratio;
  double coeffX = (left < right) ? +0.5 : -0.5;
  double coeffY = (bottom < top) ? +0.5 : -0.5;
  double coeffZ = (zNear < zFar) ? +0.5 : -0.5;
  *this = GLOrthoParams(
    center[0] - coeffX*size[0], center[0] + coeffX*size[0],
    center[1] - coeffY*size[1], center[1] + coeffY*size[1],
    center[2] - coeffZ*size[2], center[2] + coeffZ*size[2]);
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
void GLOrthoParams::fixAspectRatio(const Viewport& old_value, const Viewport& new_value)
{
  int OldW = old_value.width, NewW = new_value.width;
  int OldH = old_value.height, NewH = new_value.height;

  if (!NewW || !NewH)
    return;

  if (OldW && OldH)
    scaleAroundCenter(Point3d(NewW / (double)OldW, NewH / (double)OldH, 1.0), getCenter());

  fixAspectRatio((double)NewW / (double)NewH);
}

////////////////////////////////////////////////////////////////////////
void GLOrthoParams::writeToObjectStream(ObjectStream& out) 
{
  out.writeString("left", cstring(left));
  out.writeString("right", cstring(right));
  out.writeString("bottom", cstring(bottom));
  out.writeString("top", cstring(top));
  out.writeString("zNear", cstring(zNear));
  out.writeString("zFar", cstring(zFar));
}

////////////////////////////////////////////////////////////////////////
void GLOrthoParams::readFromObjectStream(ObjectStream& in) 
{
  left   = cdouble(in.readString("left"));
  right  = cdouble(in.readString("right"));
  bottom = cdouble(in.readString("bottom"));
  top    = cdouble(in.readString("top"));
  zNear  = cdouble(in.readString("zNear"));
  zFar   = cdouble(in.readString("zFar"));
}


} //namespace Visus