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

#include <Visus/PointQuery.h>
#include <Visus/Dataset.h>

namespace Visus {

////////////////////////////////////////////////////////////////////
bool PointQuery::setPoints(PointNi nsamples)
{
  //no samples or overflow
  if (nsamples.innerProduct() <= 0)
    return false;

  if (!this->logic_position.valid())
    return false;

  if (!this->points.resize(nsamples,DTypes::INT64_RGB, __FILE__, __LINE__))
    return false;

  //definition of a point query!
  //P'=T* (P0 + I* X/nsamples[0] +  J * Y/nsamples[1] + K * Z/nsamples[2])
  //P'=T*P0 +(T*Stepx)*I + (T*Stepy)*J + (T*Stepz)*K

  auto T   = this->logic_position.getTransformation().withSpaceDim(4);
  auto box = this->logic_position.getBoxNd().withPointDim(3);

  Point4d P0(box.p1[0], box.p1[1], box.p1[2], 1.0);
  Point4d X(1, 0, 0, 0); X[0] = box.p2[0] - box.p1[0]; Point4d DX = X * (1.0 / (double)nsamples[0]); VisusAssert(X[3] == 0.0 && DX[3] == 0.0);
  Point4d Y(0, 1, 0, 0); Y[1] = box.p2[1] - box.p1[1]; Point4d DY = Y * (1.0 / (double)nsamples[1]); VisusAssert(Y[3] == 0.0 && DY[3] == 0.0);
  Point4d Z(0, 0, 1, 0); Z[2] = box.p2[2] - box.p1[2]; Point4d DZ = Z * (1.0 / (double)nsamples[2]); VisusAssert(Z[3] == 0.0 && DZ[3] == 0.0);

  Point4d TP0_4d = T * P0;                                Point3d TP0 = TP0_4d.dropHomogeneousCoordinate();
  Point4d TDX_4d = T * DX; VisusAssert(TDX_4d[3] == 0.0); Point3d TDX = TDX_4d.toPoint3();
  Point4d TDY_4d = T * DY; VisusAssert(TDY_4d[3] == 0.0); Point3d TDY = TDY_4d.toPoint3();
  Point4d TDZ_4d = T * DZ; VisusAssert(TDZ_4d[3] == 0.0); Point3d TDZ = TDZ_4d.toPoint3();

  auto DST = (Int64*)this->points.c_ptr();
  Point3d PZ = TP0; for (int K = 0; K < nsamples[2]; ++K, PZ += TDZ) {
  Point3d PY = PZ;  for (int J = 0; J < nsamples[1]; ++J, PY += TDY) {
  Point3d PX = PY;  for (int I = 0; I < nsamples[0]; ++I, PX += TDX) {
    *DST++ = (Int64)(PX[0]);
    *DST++ = (Int64)(PX[1]);
    *DST++ = (Int64)(PX[2]);
  }}}

  return true;
}

////////////////////////////////////////////////////////////////////
bool PointQuery::allocateBufferIfNeeded()
{
  auto nsamples = getNumberOfSamples();

  if (!buffer)
  {
    if (!buffer.resize(nsamples, field.dtype, __FILE__, __LINE__))
      return false;

    buffer.fillWithValue(field.default_value);
    buffer.layout = field.default_layout;
  }

  //check buffer
  VisusAssert(buffer.dtype == field.dtype);
  VisusAssert(buffer.c_size() == getByteSize());

  //this covers the case when the user specify a 3d array for a 3d datasets
  //or for pointqueries 
#if 1
  VisusAssert(buffer.dims.innerProduct() == nsamples.innerProduct());
  buffer.dims = nsamples;
#endif

  return true;
}

} //namespace Visus

