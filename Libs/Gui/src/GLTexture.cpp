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
class GLTexture::Pimpl 
{
public:

  QOpenGLTexture* tex;

  //constructor
  Pimpl(
    QOpenGLTexture::Target target, 
    int width, int height, int depth, 
    QOpenGLTexture::TextureFormat textureFormat, 
    QOpenGLTexture::PixelFormat sourceFormat, QOpenGLTexture::PixelType sourceType, 
    const void* c_ptr) 
  {
    tex = new QOpenGLTexture(target);

    if (!tex->create())
    {
      delete tex;
      tex = nullptr;
    }

    GLint originalAlignment = 1;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &originalAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    tex->setSize(width, height, depth);
    tex->setFormat(textureFormat);
    tex->setMipLevels(1);
    tex->allocateStorage();
    tex->setData(sourceFormat, sourceType, c_ptr);

    glPixelStorei(GL_UNPACK_ALIGNMENT, originalAlignment);
  }

  //destructor
  ~Pimpl()
  {
    if (tex)
      delete tex;
  }

  //textureId
  GLuint textureId() const {
    return tex ? tex->textureId() : 0;
  }

};



  ///////////////////////////////////////////////
GLTexture::GLTexture(Array src) 
{
  this->upload.array = std::make_shared<Array>(src);
  init((int)src.getWidth(), (int)src.getHeight(), (int)src.getDepth(), src.dtype);
}

  ///////////////////////////////////////////////
GLTexture::GLTexture(SharedPtr<QImage> src) 
{
  src = std::make_shared<QImage>(src->mirrored());
  src = std::make_shared<QImage>(src->convertToFormat(QImage::Format_RGBA8888));
  this->upload.image = src;
  init((int)src->width(), (int)src->height(), 1, DTypes::UINT8_RGBA);
}

///////////////////////////////////////////////
GLTexture::~GLTexture()
{
  releasePimpl(true);
}

  ///////////////////////////////////////////////
void GLTexture::releasePimpl(bool bDeleteTexture)
{
  if (!this->pimpl)
    return;
  
  auto pimpl = this->pimpl;
  auto size  = this->dtype.getByteSize((Int64)this->dims.x*(Int64)this->dims.y*(Int64)this->dims.z);
  this->pimpl=nullptr;
    
  if (auto do_with_context=GLDoWithContext::getSingleton())
  {
    do_with_context->push_back([size, pimpl, bDeleteTexture]()
    {
      GLInfo::getSingleton()->freeMemory(size);
      if (bDeleteTexture)
      	delete pimpl;
    });
  }
}

  
///////////////////////////////////////////////
void GLTexture::init(int width, int height, int depth, DType dtype)
{
  VisusAssert(dtype.valid());

  int ncomponents = dtype.ncomponents();

  this->dims = Point3i(width, height, depth);
  this->dtype = dtype;

  if (dtype.isVectorOf(DTypes::UINT8))
  {
    this->sourceType = QOpenGLTexture::UInt8;

    switch (ncomponents)
    {
    case 1: sourceFormat = QOpenGLTexture::Luminance;      textureFormat = QOpenGLTexture::LuminanceFormat; break;
    case 2: sourceFormat = QOpenGLTexture::LuminanceAlpha; textureFormat = QOpenGLTexture::LuminanceAlphaFormat; break;
    case 3: sourceFormat = QOpenGLTexture::RGB;            textureFormat = QOpenGLTexture::RGBFormat; break;
    case 4: sourceFormat = QOpenGLTexture::RGBA;           textureFormat = QOpenGLTexture::RGBAFormat; break;
    default:VisusInfo() << "Failed to upload texture internal error"; VisusAssert(false); break;
    }
  }
  //disabled Uint16 texturing... they seems broken, try http://atlantis.sci.utah.edu/mod_visus?dataset=MM336-001
#if 0
  else if (dtype.isVectorOf(DTypes::UINT16))
  {
    this->sourceType = QOpenGLTexture::UInt16;

    switch (ncomponents)
    {
    case 1: sourceFormat = QOpenGLTexture::Luminance;      textureFormat = QOpenGLTexture::RGB16U; break; //missing LuminanceFormat16U???
    case 2: sourceFormat = QOpenGLTexture::LuminanceAlpha; textureFormat = QOpenGLTexture::RGBA16U; break; //missing LuminanceAlphaFormat16U?
    case 3: sourceFormat = QOpenGLTexture::RGB;            textureFormat = QOpenGLTexture::RGB16U; break;
    case 4: sourceFormat = QOpenGLTexture::RGBA;           textureFormat = QOpenGLTexture::RGBA16U; break;
    default: VisusInfo() << "Failed to upload texture internal error"; VisusAssert(false); break;
    }
  }
#endif
  else
  {
    this->sourceType = QOpenGLTexture::Float32;

    //force texture to be FLOAT32 !
    this->dtype = DType(ncomponents, DTypes::FLOAT32);

    switch (ncomponents)
    {
    case 1: sourceFormat = QOpenGLTexture::Luminance;      textureFormat = QOpenGLTexture::RGB32F; break; //missing LuminanceFormat32F???
    case 2: sourceFormat = QOpenGLTexture::LuminanceAlpha; textureFormat = QOpenGLTexture::RGBA32F; break; //missing LuminanceAlphaFormat32F???
    case 3: sourceFormat = QOpenGLTexture::RGB;            textureFormat = QOpenGLTexture::RGB32F; break;
    case 4: sourceFormat = QOpenGLTexture::RGBA;           textureFormat = QOpenGLTexture::RGBA32F; break;
    default: VisusInfo() << "Failed to upload texture internal error"; VisusAssert(false); break;
    }
  }
}

///////////////////////////////////////////////
GLuint GLTexture::textureId() 
{
  if (pimpl)
    return pimpl->textureId();

  //already failed
  if (!upload.array && !upload.image)
    return 0;

  //try the upload only once
  auto array = upload.array; upload.array.reset();
  auto image = upload.image; upload.image.reset();

  //cast needed
  if (array && this->dtype != array->dtype)
  {
    auto casted = ArrayUtils::cast(*array, this->dtype);
    if (!casted) return 0;
    array = std::make_shared<Array>(casted);
  }

  auto size = this->dtype.getByteSize((Int64)this->dims.x*(Int64)this->dims.y*(Int64)this->dims.z);

  if (!GLInfo::getSingleton()->allocateMemory(size))
  {
    VisusInfo() << "Failed to upload texture Failed to allocate gpu memory (" + StringUtils::getStringFromByteSize(size) + "), GLCreateTexture::setArray failed";
    return 0;
  }

  const uchar* c_ptr = array ? (const uchar*)array->c_ptr() : image->constBits();

  this->pimpl = new Pimpl(this->target(),width(),height(),depth(),textureFormat,sourceFormat,sourceType,c_ptr);

  auto ret = pimpl->textureId();
  if (!ret)
  {
    VisusInfo() << "Failed to create texture";
    delete pimpl;
    pimpl = nullptr;
    GLInfo::getSingleton()->freeMemory(size);
  }

  return ret;
}

} //namespace

