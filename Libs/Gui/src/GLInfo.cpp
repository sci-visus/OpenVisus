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

#include <Visus/GLInfo.h>
#include <Visus/GLCanvas.h>
#include <Visus/StringTree.h>

#if __clang__ && __APPLE__
  #include <OpenGL/CGLTypes.h>
  #include <OpenGL/OpenGL.h>
  #include <IOKit/graphics/IOGraphicsLib.h>
  #include <CoreFoundation/CoreFoundation.h>
  #include <ApplicationServices/ApplicationServices.h>
#endif

#ifndef GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX
#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#endif

#ifndef GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049
#endif 

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(GLInfo)

///////////////////////////////////////////////////////////////
#if __clang__ && __APPLE__
Int64 getTotalVideoMemoryBytes()
{
  CGLRendererInfoObj info;
  GLint infoCount = 0;

  CGLError e = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()), &info, &infoCount);
  if (e != kCGLNoError)
  {
    PrintError(CGLErrorString(e));
    return -1;
  }

  Int64 tot=0;
  for (int i = 0; i < infoCount; ++i)
  {
    GLint mega;
    CGLDescribeRenderer(info, i, kCGLRPVideoMemoryMegabytes, &mega);
    //PrintInfo("vid mem: ",mega,"MB");
    tot=mega;
    CGLDescribeRenderer(info, i, kCGLRPTextureMemoryMegabytes, &mega);
    //PrintInfo("tex mem:",mega,"MB");
    if (tot > 0) break;  // software renderer always reports 0 bytes, so break when we find a renderer with memory
  }

  CGLDestroyRendererInfo(info);
  return tot*1024*1024;
}

//NOTE: does not return dependable values for integrated Intel HD agus
static Int64 ReadPerfInt64Value(CFStringRef name)
{
  Int64 ret=0;
  kern_return_t krc;
  mach_port_t masterPort;
  krc = IOMasterPort(bootstrap_port, &masterPort);
  if (krc == KERN_SUCCESS)
  {
    CFMutableDictionaryRef pattern = IOServiceMatching(kIOAcceleratorClassName);//CFShow(pattern);
      
    io_iterator_t deviceIterator;
    krc = IOServiceGetMatchingServices(masterPort, pattern, &deviceIterator);
    if (krc == KERN_SUCCESS)
    {
      io_object_t object;
      while (!ret && (object = IOIteratorNext(deviceIterator)))
      {
        CFMutableDictionaryRef properties = nullptr;
        krc = IORegistryEntryCreateCFProperties(object, &properties, kCFAllocatorDefault, (IOOptionBits)0);
        if (krc == KERN_SUCCESS)
        {
          CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( properties, CFSTR("PerformanceStatistics") );//CFShow(perf_properties);
          if (const void* vramUsedBytes = CFDictionaryGetValue(perf_properties, name))
            CFNumberGetValue( (CFNumberRef) vramUsedBytes, kCFNumberSInt64Type, &ret);
        }
        if (properties) CFRelease(properties);
        if (object    ) IOObjectRelease(object);
      }
      if (deviceIterator) IOObjectRelease(deviceIterator);
    }
  }
  return ret;
}
#endif 


///////////////////////////////////////////////
GLInfo::GLInfo() : gpu_total_memory(0), gpu_free_memory(0), gpu_used_memory(0)
{
  GLNeedContext gl;

  this->vendor    =(char*)glGetString(GL_VENDOR);
  this->renderer  =(char*)glGetString(GL_RENDERER);
  this->version   =(char*)glGetString(GL_VERSION);
  this->extensions=(char*)glGetString(GL_EXTENSIONS);

  glGetIntegerv(GL_RED_BITS, &this->red_bits);
  glGetIntegerv(GL_GREEN_BITS, &this->green_bits);
  glGetIntegerv(GL_BLUE_BITS, &this->blue_bits);
  glGetIntegerv(GL_ALPHA_BITS, &this->alpha_bits);
  glGetIntegerv(GL_DEPTH_BITS, &this->depth_bits);
  glGetIntegerv(GL_STENCIL_BITS, &this->stencil_bits);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &this->max_texture_size);
  
  #ifdef GL_MAX_LIGHTS
  glGetIntegerv(GL_MAX_LIGHTS, &this->max_lights);
  #endif

  #ifdef GL_MAX_3D_TEXTURE_SIZE
  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &this->max_3d_texture_size); 
  #endif

  #ifdef GL_MAX_CLIP_PLANES
  glGetIntegerv(GL_MAX_CLIP_PLANES, &this->max_clip_planes);
  #endif

  //GPU memory
  #if __clang__ && __APPLE__
  {
    this->gpu_total_memory = getTotalVideoMemoryBytes();
    this->gpu_free_memory  = this->gpu_total_memory - ReadPerfInt64Value(CFSTR("vramUsedBytes"));
    this->gpu_used_memory = 0;
  }
  #else
  {
    if (StringUtils::contains(this->extensions, "GL_NVX_gpu_memory_info"))
    {
      GLint kb = 0;

      glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &kb);   VisusAssert(kb > 0);
      this->gpu_total_memory = 1024 * (Int64)kb;

      glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &kb); VisusAssert(kb > 0);
      this->gpu_free_memory = 1024 * (Int64)kb;

      //no used memory so far
      this->gpu_used_memory = 0;
    }
    else
    {
      this->gpu_total_memory = StringUtils::getByteSizeFromString("1024GB"); //no information provided, pick a very high value for GPU (1024GB)
      this->gpu_free_memory = Int64(gpu_total_memory);
      this->gpu_used_memory = 0;
    }
  }
  #endif
}


//////////////////////////////////////////////////////////////////////
void GLInfo::setGpuTotalMemory(Int64 value) {
  this->gpu_total_memory = value;
  this->gpu_free_memory = value;
  this->gpu_used_memory = 0;
}

/// ///////////////////////////////////////////////////////////////////
bool GLInfo::mallocOpenGLMemory(Int64 size, bool simulate_only)
{
  if (simulate_only)
    return (this->gpu_free_memory - size) >= 0;

  //consider atomicity
  this->gpu_used_memory += size;
  this->gpu_free_memory -= size;
  
  if (this->gpu_free_memory < 0)
  {
    this->gpu_used_memory -= size;
    this->gpu_free_memory += size;
    PrintInfo("mallocOpenGLMemory failed, not enough space", "requested", StringUtils::getStringFromByteSize(size), "free", StringUtils::getStringFromByteSize(gpu_free_memory));
    return false;
  }
  else
  {
    //PrintInfo("GPU Allocated", StringUtils::getStringFromByteSize(size));
    return true;
  }
}

/// ///////////////////////////////////////////////////////////////////
void GLInfo::freeOpenGLMemory(Int64 size)
{
  //PrintInfo("GPU Freeed", StringUtils::getStringFromByteSize(size));
  this->gpu_used_memory -= size;
  this->gpu_free_memory += size;
}

} //namespace

