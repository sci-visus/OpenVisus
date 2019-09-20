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
  auto size = this->dtype.getByteSize((Int64)this->dims[0]*(Int64)this->dims[1]*(Int64)this->dims[2]);
  this->texture_id = 0;

  if (auto do_with_context = GLDoWithContext::getSingleton())
  {
    do_with_context->push_back([size, texture_id]()
    {
      //GLInfo::getSingleton()->freeOpenGLMemory(size);
      glDeleteTextures(1, &texture_id);
    });
  }
}

///////////////////////////////////////////////
GLuint GLTexture::textureId(GLCanvas& gl)
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

  auto target = this->target();
  auto ncomponents = this->dtype.ncomponents();
  auto fullsize = this->dtype.getByteSize((Int64)this->dims[0] * (Int64)this->dims[1] * (Int64)this->dims[2]);

  const int uint8_textureFormats[][5] = {
    {0, GL_LUMINANCE ,GL_LUMINANCE_ALPHA,GL_RGB ,GL_RGBA},                    //NoCompression
    {0,0,0,GL_COMPRESSED_RGB,GL_COMPRESSED_RGBA},                             //GenericCompression
    {0,0,0,GL_COMPRESSED_RGB_S3TC_DXT1_EXT,GL_COMPRESSED_RGBA_S3TC_DXT5_EXT}, //S3TCCompression
    {0,0,0,GL_COMPRESSED_RGB8_ETC2,GL_COMPRESSED_RGBA8_ETC2_EAC},             //ETC2Compression
  };

  //todo: make sure they works...
  const int float32_textureFormats[][5] = {
    {0, GL_LUMINANCE32F_ARB ,GL_LUMINANCE_ALPHA32F_ARB,GL_RGB32F,GL_RGBA32F }, //NoCompression
    {0,0,0,GL_COMPRESSED_RGB,GL_COMPRESSED_RGBA},                              //GenericCompression 
    {0,0,0,GL_COMPRESSED_RGB_S3TC_DXT1_EXT,GL_COMPRESSED_RGBA_S3TC_DXT5_EXT},  //S3TCCompression    
    {0,0,0,GL_COMPRESSED_RGB8_ETC2,GL_COMPRESSED_RGBA8_ETC2_EAC},              //ETC2Compression    
  };

  //this->compression = S3TCCompression;

  const auto& textureFormats = dtype.isVectorOf(DTypes::UINT8) ?
    uint8_textureFormats :
    float32_textureFormats;

  auto textureFormat = (QOpenGLTexture::TextureFormat)textureFormats[this->compression][ncomponents];
  if (!textureFormat)
  {
    this->compression = NoCompression;
    textureFormat = (QOpenGLTexture::TextureFormat)textureFormats[this->compression][ncomponents];
  }

  //i cannot know in advance with texture compression how much memory i'm going to use
#if 0
  if (!GLInfo::getSingleton()->allocateOpenGLMemory(fullsize))
  {
    VisusInfo() << "Failed to upload texture Failed to allocate gpu memory (" + StringUtils::getStringFromByteSize(fullsize) + "), GLCreateTexture::setArray failed";
    return 0;
  }
#endif

  auto sourceFormat = std::vector<int>({ 0,GL_LUMINANCE,GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA })[ncomponents];
  auto sourceType = dtype.isVectorOf(DTypes::UINT8) ? GL_UNSIGNED_BYTE : GL_FLOAT;

  GLint save_active_texture;
  gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &save_active_texture);

  GLint save_original_alignment = 1;
  gl.glGetIntegerv(GL_UNPACK_ALIGNMENT, &save_original_alignment);

  gl.glGenTextures(1, &texture_id);

  if (texture_id)
  {
    gl.flushGLErrors(false);

    if(save_active_texture != GL_TEXTURE0)
      gl.glActiveTexture(GL_TEXTURE0);

    gl.glBindTexture(target, texture_id);

    if (save_original_alignment != 1)
      gl.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    //seems important since I'm not using mipmaps
    gl.glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (target == QOpenGLTexture::Target3D)
      gl.glTexImage3D(target, 0, textureFormat, dims[0], dims[1], dims[2], 0, sourceFormat, sourceType, pixels);
    else
      gl.glTexImage2D(target, 0, textureFormat, dims[0], dims[1], 0, sourceFormat, sourceType, pixels);
  }

  if (!texture_id || gl.flushGLErrors(true))
  {
    VisusInfo() << "Failed to create texture";

    //see not above for allocate Memory
#if 0
    GLInfo::getSingleton()->freeOpenGLMemory(fullsize);
#endif

    if (texture_id)
    {
      gl.glDeleteTextures(1, &texture_id);
      texture_id = 0;
    }
  }

  if (texture_id)
  {
    GLint gpusize = fullsize;
    if (compression != NoCompression)
    {
      GLint compressed;
      glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
      if (compressed == GL_TRUE)
        glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &gpusize);
    }

    GLInfo::getSingleton()->addVisusUsedMemory(gpusize);

    VisusInfo() << (gpusize==fullsize?"Non compressed":"Compressed")<<" texture"
      << " fullsize(" << StringUtils::getStringFromByteSize(fullsize) << ")"
      << " gpusize(" << gpusize << ")"
      << " ratio(" << (100.0 * double(gpusize) / double(fullsize)) << "%)";
  }

  if (save_original_alignment!=1)
    gl.glPixelStorei(GL_UNPACK_ALIGNMENT, save_original_alignment);

  if (save_active_texture!= GL_TEXTURE0)
    gl.glActiveTexture(save_active_texture);

  return texture_id;
}

} //namespace

