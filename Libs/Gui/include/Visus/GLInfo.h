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

#ifndef __VISUS_GL_INFO_H
#define __VISUS_GL_INFO_H

#include <Visus/Gui.h>
#include <Visus/CriticalSection.h>

namespace Visus {

////////////////////////////////////////////////////
class VISUS_GUI_API GLInfo
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(GLInfo)

  String vendor;
  String renderer;
  String version;
  String extensions;

  int    red_bits=0;
  int    green_bits=0;
  int    blue_bits=0;
  int    alpha_bits=0;
  int    depth_bits=0;
  int    stencil_bits=0;
  int    max_lights=0;
  int    max_texture_size=0;
  int    max_3d_texture_size=0;
  int    max_clip_planes=0;
  int    gles_version=0;  //(0 for desktop Opengl, or 2 or 3 or...)

  //hasTexture3D
  bool hasTexture3D() const
  {return gles_version==0 || gles_version>=3;}

  //getGpuTotalMemory
  Int64 getGpuTotalMemory() const {
    return gpu_total_memory;
  }

  //getGpuUsedMemory
  Int64 getGpuUsedMemory() {
    return gpu_used_memory;
  }

  //setOsTotalMemory
  Int64 getGpuFreeMemory() {
    return gpu_free_memory;
  }

public:

  //setGpuTotalMemory
  void setGpuTotalMemory(Int64 value);

  bool mallocOpenGLMemory(Int64 size, bool simulate_only=false);

  //freeOpenGLMemory
  void freeOpenGLMemory(Int64 size);

private:

  std::atomic<Int64> gpu_total_memory;
  std::atomic<Int64> gpu_used_memory;
  std::atomic<Int64> gpu_free_memory;

  //constructor
  GLInfo();

};


} //namespace

#endif //__VISUS_GL_INFO_H