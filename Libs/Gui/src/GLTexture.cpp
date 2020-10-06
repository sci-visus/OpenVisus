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
SharedPtr<GLTexture> GLTexture::createFromArray(Array src)
{
  int ncomponents = src.dtype.ncomponents();

  if (!(src.dtype.valid() && ncomponents >= 1 && ncomponents <= 4))
  {
    //PrintInfo("Failed to upload texture internal error");
    //VisusAssert(false);
    return SharedPtr<GLTexture>();
  }

  auto glinfo = GLInfo::getSingleton();
  if (glinfo->getGpuTotalMemory() && src.c_size() > glinfo->getGpuFreeMemory())
  {
    PrintInfo("failed to create Texture, not enough memory", "requested", StringUtils::getStringFromByteSize(src.c_size()));
    return SharedPtr<GLTexture>();
  }

  //NOTE: I cannot upload here since I don't have the OpenGL context, so I keep a copy of the memory
  //to upload later
  auto ret = std::make_shared<GLTexture>();
  ret->upload.array = src;
  ret->dims = Point3i((int)src.getWidth(), (int)src.getHeight(), (int)src.getDepth());

  if (src.dtype.isVectorOf(DTypes::UINT8))
    ret->dtype = DType(ncomponents, DTypes::UINT8);
  else
    ret->dtype = DType(ncomponents, DTypes::FLOAT32);

  return ret;
}

///////////////////////////////////////////////
SharedPtr<GLTexture>GLTexture::createFromQImage(QImage src)
{
  src = src.mirrored();
  src = src.convertToFormat(QImage::Format_RGBA8888);

  auto ret = std::make_shared<GLTexture>();
  ret->upload.image = src;
  ret->dims = Point3i((int)src.width(), (int)src.height(), 1);
  ret->dtype = DTypes::UINT8_RGBA;
  return ret;
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
    do_with_context->push_back([texture_id]()
    {
      //GLInfo::getSingleton()->freeOpenGLMemory(size);
      glDeleteTextures(1, &texture_id);
    });
  }
}

///////////////////////////////////////////////
//scrgiorgio: 0 means to disable compression (i.e. that kind of compression is not supported)

#ifndef GL_COMPRESSED_RGB
#define GL_COMPRESSED_RGB 0
#endif

#ifndef GL_COMPRESSED_RGBA
#define GL_COMPRESSED_RGBA 0
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0
#endif

#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0
#endif

#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0
#endif

#ifndef GL_LUMINANCE32F_ARB
#define GL_LUMINANCE32F_ARB 0
#endif

#ifndef GL_LUMINANCE_ALPHA32F_ARB
#define GL_LUMINANCE_ALPHA32F_ARB 0
#endif

#ifndef GL_RGB32F
#define GL_RGB32F 0
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0
#endif

GLuint GLTexture::textureId(GLCanvas& gl)
{
  if (texture_id)
    return texture_id;

  //already failed
  if (!upload.array.valid() && upload.image.width() == 0)
    return 0;

  String failure_texture;

  //try the upload only once
  auto array = upload.array; upload.array = Array();
  auto image = upload.image; upload.image = QImage();

  const uchar* pixels = nullptr;
  if (array.valid())
  {
    if (this->dtype != array.dtype)
    {
      auto casted = ArrayUtils::cast(array, this->dtype);
      if (!casted.valid())
      {
        PrintInfo( "Texture generation failed (failed to creatre casted array)");
        return 0;
      }
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

  auto glinfo = GLInfo::getSingleton();

  if (glinfo->getGpuTotalMemory() && fullsize > glinfo->getGpuFreeMemory())
  {
    PrintInfo("Failed to upload texture Not enough VRAM");
    return 0;
  }

  auto sourceFormat = std::vector<int>({ 0,GL_LUMINANCE,GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA })[ncomponents];
  auto sourceType = dtype.isVectorOf(DTypes::UINT8) ? GL_UNSIGNED_BYTE : GL_FLOAT;

  GLint save_active_texture;
  gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &save_active_texture);

  GLint save_original_alignment = 1;
  gl.glGetIntegerv(GL_UNPACK_ALIGNMENT, &save_original_alignment);

  gl.glGenTextures(1, &texture_id);

  if (!texture_id)
  {
    failure_texture = "glGenTextures failed";
  }
  else
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
    PrintInfo("Failed to create texture");

    //see not above for allocate Memory
#if 0
    GLInfo::getSingleton()->freeOpenGLMemory(fullsize);
#endif

    if (texture_id)
    {
      gl.glDeleteTextures(1, &texture_id);
      failure_texture = "glTexImage<n>D failed";
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

    if (false)
    {
      PrintInfo(gpusize == fullsize ? "Non compressed" : "Compressed", "texture",
        "fullsize", StringUtils::getStringFromByteSize(fullsize),
        "gpusize", gpusize,
        "ratio", (100.0 * double(gpusize) / double(fullsize)), "%");
    }
  }

  if (save_original_alignment!=1)
    gl.glPixelStorei(GL_UNPACK_ALIGNMENT, save_original_alignment);

  if (save_active_texture!= GL_TEXTURE0)
    gl.glActiveTexture(save_active_texture);

  if (!texture_id)
    PrintInfo("Texture generation failed (",failure_texture,")");

  return texture_id;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
Range ComputeRange(Array data, int C, int normalization, Range user_range, bool bNormalizeToFloat)
{
  Range range;

  if (normalization == Palette::UserRange)
  {
    range = user_range;
  }
  else if (normalization == Palette::FieldRange)
  {
    range = data.dtype.getDTypeRange(C);
    if (range.delta() <= 0)
    {
      if (data.dtype.isDecimal())
        range = ArrayUtils::computeRange(data, C); //just use the computer range of the data (the c++ range would be too high)
      else
        range = GetCppRange(data.dtype); //assume the data is spread in all the possible discrete range
    }
  }
  else if (normalization == Palette::ComputeRangePerComponent)
  {
    range = ArrayUtils::computeRange(data, C);
  }
  else if (normalization == Palette::ComputeRangeOverall)
  {
    range = Range::invalid();
    for (int C = 0; C < data.dtype.ncomponents(); C++)
      range = range.getUnion(ArrayUtils::computeRange(data, C));
  }
  else
  {
    ThrowException("internal error");
  }

  //the GL textures is read from GPU memory always in the range [0,1]
    //see https://www.khronos.org/opengl/wiki/Normalized_Integer
  if (bNormalizeToFloat && !data.dtype.isDecimal() && range.delta())
  {
    auto cpp_range = GetCppRange(data.dtype);
    auto A = cpp_range.from;
    auto B = cpp_range.to;

    auto C = data.dtype.isUnsigned() ? 0.0 : -1.0;
    auto D = 1.0;

    auto NormalizeInteger = [&](double value) {
      auto alpha = (value - A) / (B - A);
      return C + alpha * (D - C);
    };

    range = Range(
      NormalizeInteger(range.from),
      NormalizeInteger(range.to),
      0.0);
  }

  return range;
}

} //namespace

