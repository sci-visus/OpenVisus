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

#if __APPLE__
  #include <OpenGL/CGLTypes.h>
  #include <OpenGL/OpenGL.h>
  #include <IOKit/graphics/IOGraphicsLib.h>
  #include <CoreFoundation/CoreFoundation.h>
  #include <ApplicationServices/ApplicationServices.h>
#endif

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(GLInfo)

///////////////////////////////////////////////////////////////
#if __APPLE__
Int64 getTotalVideoMemoryBytes()
{
  CGLRendererInfoObj info;
  GLint infoCount = 0;

  CGLError e = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()), &info, &infoCount);
  if (e != kCGLNoError)
  {
    VisusError()<<CGLErrorString(e);
    return -1;
  }

  Int64 tot=0;
  for (int i = 0; i < infoCount; ++i)
  {
    GLint mega;
    CGLDescribeRenderer(info, i, kCGLRPVideoMemoryMegabytes, &mega);
    VisusInfo()<<"vid mem: "<<mega<<" MB";
    tot=mega;
    CGLDescribeRenderer(info, i, kCGLRPTextureMemoryMegabytes, &mega);
    VisusInfo()<<"tex mem: "<<mega<<" MB";
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
#endif //__APPLE__


///////////////////////////////////////////////
GLInfo::GLInfo() : visus_used_memory(0),os_total_memory(0),extension_GL_NVX_gpu_memory_info(false)
{
  VisusInfo() << "GLCanvas::initOpenGL...";

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

  VisusInfo()<<"gles_version "          <<this->gles_version;
  VisusInfo()<<"GL_VENDOR "             <<this->vendor;
  VisusInfo()<<"GL_RENDERER "           <<this->renderer;
  VisusInfo()<<"GL_VERSION "            <<this->version;
  //VisusInfo()<<"GL_EXTENSIONS "         <<this->extensions;
  VisusInfo()<<"GL_RED_BITS "           <<this->red_bits;
  VisusInfo()<<"GL_GREEN_BITS "         <<this->green_bits;
  VisusInfo()<<"GL_BLUE_BITS "          <<this->blue_bits;
  VisusInfo()<<"GL_ALPHA_BITS "         <<this->alpha_bits;
  VisusInfo()<<"GL_DEPTH_BITS "         <<this->depth_bits;
  VisusInfo()<<"GL_STENCIL_BITS "       <<this->stencil_bits;
  VisusInfo()<<"GL_MAX_LIGHTS "         <<this->max_lights;
  VisusInfo()<<"GL_MAX_TEXTURE_SIZE "   <<this->max_texture_size;
  VisusInfo()<<"GL_MAX_3D_TEXTURE_SIZE "<<this->max_3d_texture_size;
  VisusInfo()<<"GL_MAX_CLIP_PLANES "    <<this->max_clip_planes;

  #if __APPLE__
  {
    this->os_total_memory=getTotalVideoMemoryBytes();
  }
  #else
  {
    this->extension_GL_NVX_gpu_memory_info=StringUtils::contains(this->extensions,"GL_NVX_gpu_memory_info");
    if (extension_GL_NVX_gpu_memory_info)
    {
      #ifndef GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX
      #define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
      #endif
      GLint kb=0;
      glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX,&kb); VisusAssert(kb>0);
      this->os_total_memory=1024*(Int64)kb;
    }
  }
  #endif

  VisusInfo() << "...done";
}

//////////////////////////////////////////////////////////
Int64 GLInfo::getGpuUsedMemory()
{
  #if __APPLE__
  {
    return ReadPerfInt64Value(CFSTR("vramUsedBytes"));
  }
  #else
  {
    Int64 ret=0;
    GLNeedContext gl;
    if (extension_GL_NVX_gpu_memory_info)
    {
      #ifndef GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX
      #define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049
      #endif 
      GLint kb=0;
      glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX,&kb); VisusAssert(kb>0);
      ret=os_total_memory-(kb*(Int64)1024);
    }
    return ret;
  }
  #endif

}

///////////////////////////////////////////////
Int64 GLInfo::getVisusUsedMemory()
{
  ScopedLock lock(this->lock);
  return visus_used_memory;
}

///////////////////////////////////////////////
Int64 GLInfo::getOsTotalMemory()
{
  return os_total_memory;
}

///////////////////////////////////////////////
void GLInfo::setOsTotalMemory(Int64 value)
{
  os_total_memory=value;
}

///////////////////////////////////////////////
#if 0
bool GLInfo::allocateOpenGLMemory(Int64 reqsize)
{
  VisusAssert(reqsize>=0);

  if (!reqsize) 
  {
    return true;
  }
  else
  {
    ScopedLock lock(this->lock);

    if (os_total_memory>0)
    {
      Int64 os_free_memory=((Int64)(getOsTotalMemory()*0.80))-visus_used_memory;
      if (reqsize>os_free_memory)
      {
        VisusWarning()<<"OpenGL out of memory "
                      <<" reqsize("<<StringUtils::getStringFromByteSize(reqsize)<<")"
                      <<" os_free_memory("<<StringUtils::getStringFromByteSize(os_free_memory)<<")";

        return false;
      }
    }

    this->visus_used_memory+=reqsize;
    return true;
  }
}
#endif

///////////////////////////////////////////////
#if 0
bool GLInfo::freeOpenGLMemory(Int64 reqsize)
{
  VisusAssert(reqsize>=0);

  if (!reqsize)
  {
    return true;
  }
  else
  {
    ScopedLock lock(this->lock);
    this->visus_used_memory-=reqsize;
    return true;
  }
}
#endif


///////////////////////////////////////////////
void GLInfo::addVisusUsedMemory(Int64 reqsize)
{
  ScopedLock lock(this->lock);
  this->visus_used_memory += reqsize;
}

} //namespace

