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

#include <Visus/GLTexture.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLInfo.h>

#include <QCoreApplication>

namespace Visus {


///////////////////////////////////////////////
GLTexture::GLTexture(Array src)
{
  int ncomponents = src.dtype.ncomponents();

  if (!(src.dtype.valid() && ncomponents >= 1 && ncomponents <= 4))
  {
    //VisusInfo() << "Failed to upload texture internal error";
    //VisusAssert(false);
    return;
  }

  this->upload.array = src;
  this->dims = Point3i((int)src.getWidth(), (int)src.getHeight(), (int)src.getDepth());

  if (src.dtype.isVectorOf(DTypes::UINT8))
    this->dtype = DType(ncomponents, DTypes::UINT8);
  else
    this->dtype = DType(ncomponents, DTypes::FLOAT32);
}

///////////////////////////////////////////////
GLTexture::GLTexture(QImage src)
{
  src = src.mirrored();
  src = src.convertToFormat(QImage::Format_RGBA8888);
  this->upload.image = src;
  this->dims = Point3i((int)src.width(), (int)src.height(), 1);
  this->dtype = DTypes::UINT8_RGBA;
}

///////////////////////////////////////////////
GLTexture::~GLTexture()
{
  if (!this->texture_id)
    return;

  auto texture_id = this->texture_id;
  auto size = this->dtype.getByteSize((Int64)this->dims.x*(Int64)this->dims.y*(Int64)this->dims.z);
  this->texture_id = 0;

  if (auto do_with_context = GLDoWithContext::getSingleton())
  {
    do_with_context->push_back([size, texture_id]()
    {
      GLInfo::getSingleton()->freeMemory(size);
      glDeleteTextures(1, &texture_id);
    });
  }
}


///////////////////////////////////////////////
GLuint GLTexture::textureId()
{
  if (texture_id)
    return texture_id;

  //already failed
  if (!upload.array && upload.image.width() == 0)
    return 0;

  //try the upload only once
  auto array = upload.array; upload.array = Array();
  auto image = upload.image; upload.image = QImage();

  const uchar* pixels = nullptr;
  if (array)
  {
    if (this->dtype != array.dtype)
    {
      auto casted = ArrayUtils::cast(array, this->dtype);
      if (!casted) return 0;
      array = casted;
    }
    pixels = (const uchar*)array.c_ptr();
  }
  else
  {
    pixels = image.constBits();
    VisusReleaseAssert(pixels);
  }

  auto size = this->dtype.getByteSize((Int64)this->dims.x*(Int64)this->dims.y*(Int64)this->dims.z);

  if (!GLInfo::getSingleton()->allocateMemory(size))
  {
    VisusInfo() << "Failed to upload texture Failed to allocate gpu memory (" + StringUtils::getStringFromByteSize(size) + "), GLCreateTexture::setArray failed";
    return 0;
  }


  auto Error = [&]() {
    VisusInfo() << "Failed to create texture";
    GLInfo::getSingleton()->freeMemory(size);
    if (texture_id)
      glDeleteTextures(1, &texture_id);
    return (texture_id = 0);
  };

  int target = this->target();

  glGenTextures(1, &texture_id);
  glBindTexture(target, texture_id);

  if (!texture_id)
    return Error();

  int ncomponents = this->dtype.ncomponents();
  int textureFormat;

  //TODO HERE! I'm wasting texture memory
  if (dtype.isVectorOf(DTypes::UINT8))
    textureFormat = (QOpenGLTexture::TextureFormat)std::vector<int>({ 0, GL_RGB8 ,GL_RGBA8  , GL_RGB8 ,GL_RGBA8 })[ncomponents];
  else
    textureFormat = (QOpenGLTexture::TextureFormat)std::vector<int>({ 0, GL_RGB32F ,GL_RGBA32F,GL_RGB32F,GL_RGBA32F })[ncomponents];

  auto sourceFormat = std::vector<int>({ 0,GL_LUMINANCE,GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA })[ncomponents];
  auto sourceType = dtype.isVectorOf(DTypes::UINT8) ? GL_UNSIGNED_BYTE : GL_FLOAT;

  GLCanvas::FlushGLErrors(false);

  GLint originalAlignment = 1;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &originalAlignment);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (target == QOpenGLTexture::Target3D)
  {
#if WIN32
    static auto glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
    static auto glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)wglGetProcAddress("glTexSubImage3D");
    VisusAssert(glTexImage3D);
    VisusAssert(glTexSubImage3D);
#endif

    glTexImage3D(target, 0, textureFormat, dims[0], dims[1], dims[2], 0, sourceFormat, sourceType, pixels);
    glTexSubImage3D(target, 0, 0, 0, 0, dims[0], dims[1], dims[2], sourceFormat, sourceType, pixels);
  }
  else
  {
    glTexImage2D(target, 0, textureFormat, dims[0], dims[1], 0, sourceFormat, sourceType, pixels);
    glTexSubImage2D(target, 0, 0, 0, dims[0], dims[1], sourceFormat, sourceType, pixels);
  }

#if __APPLE__
  glGenerateMipmap(target);
#endif

  glPixelStorei(GL_UNPACK_ALIGNMENT, originalAlignment);

  if (GLCanvas::FlushGLErrors(true))
    return Error();

  return texture_id;
}

} //namespace

