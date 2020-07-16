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

#include <Visus/MandelbrotAccess.h>
#include <Visus/Dataset.h>

namespace Visus {

  /// ///////////////////////////////////////////////////////////////
MandelbrotAccess::MandelbrotAccess(Dataset* dataset, StringTree config)
{
  this->dataset = dataset;
  this->can_read = true;
  this->can_write = false;
  this->bitsperblock = dataset->getDefaultBitsPerBlock();
}

/// ///////////////////////////////////////////////////////////////
MandelbrotAccess::~MandelbrotAccess() {
}

/// ///////////////////////////////////////////////////////////////
void MandelbrotAccess::readBlock(SharedPtr<BlockQuery> query) 
{
  const Field& field = query->field;

  //wrong field (so far only 1*float32)
  if (field.dtype != (DTypes::FLOAT32))
    return readFailed(query);

  Time t1 = Time::now();
  VisusAssert(this->bitsperblock == dataset->getDefaultBitsPerBlock());

  int samplesperblock = getSamplesPerBlock();
  int blockdim = (int)(field.dtype.getByteSize(samplesperblock));

  auto pdim = dataset->getPointDim();

  LogicSamples logic_samples = query->logic_samples;
  if (!logic_samples.valid())
    return readFailed(query);

  if (!query->allocateBufferIfNeeded())
    return readFailed(query);

  auto& buffer = query->buffer;
  buffer.layout = "";

  BoxNi   box = dataset->getLogicBox();
  PointNi dim = box.size();

  Float32* ptr = (Float32*)query->buffer.c_ptr();
  for (auto loc = ForEachPoint(buffer.dims); !loc.end(); loc.next())
  {
    PointNi pos = logic_samples.pixelToLogic(loc.pos);
    double x = (pos[0] - box.p1[0]) / (double)(dim[0]);
    double y = (pos[1] - box.p1[1]) / (double)(dim[1]);
    *ptr++ = (Float32)Mandelbrot(x, y);
  }

  return readOk(query);
}

/// ///////////////////////////////////////////////////////////////
double MandelbrotAccess::Mandelbrot(double x, double y)
{
  const double scale = 2;
  const double center_x = 0;
  const double center_y = 0;
  const int iter = 48;

  double c_x = 1.3333 * (x - 0.5) * scale - center_x;
  double c_y = (y - 0.5) * scale - center_y;
  double z_x = c_x;
  double z_y = c_y;
  int i; for (i = 0; i < iter; i++, z_x = x, z_y = y)
  {
    x = (z_x * z_x - z_y * z_y) + c_x;
    y = (z_y * z_x + z_x * z_y) + c_y;
    if ((x * x + y * y) > 4.0) return double(i) / iter;
  }
  return 0.0;
}


} //namespace Visus 

