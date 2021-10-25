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

#ifndef __VISUS_GL_TEXTURE_H
#define __VISUS_GL_TEXTURE_H

#include <Visus/Gui.h>
#include <Visus/KdArray.h>
#include <QOpenGLTexture>
#include <Visus/TransferFunction.h>

namespace Visus {

//predeclaration
class GLCanvas;

///////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLTexture : public DynObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLTexture)

  enum Compression
  {
    NoCompression = 0,
    GenericCompression,
    S3TCCompression,
    ETC2Compression
  };

  Point3i                       dims;
  DType                         dtype;
  int                           minfilter= GL_LINEAR;
  int                           magfilter= GL_LINEAR;
  int                           wrap     = GL_CLAMP_TO_EDGE;
  int                           envmode  = GL_REPLACE;
  std::vector<Range>            ranges = std::vector<Range>(4, Range(0.0,1.0,0.0));
  Compression                   compression = NoCompression;

  //constructor
  GLTexture() {
  }

  //destructor
  virtual ~GLTexture();

  //create
  static SharedPtr<GLTexture> createFromArray(Array src);

  //create
  static SharedPtr<GLTexture> createFromQImage(QImage src);
  
  //width
  int width() const {return dims[0];}

  //height
  int height() const {return dims[1];}

  //depth
  int depth() const {return dims[2];}

  //target
  QOpenGLTexture::Target target() const {
    return depth() > 1 ? QOpenGLTexture::Target3D : QOpenGLTexture::Target2D;
  }

  //uploaded
  bool uploaded() const {
    return texture_id > 0;
  }

  //textureId (call only when you have a context)
  GLuint textureId(GLCanvas& gl);

private:

  struct
  {
    Array  array;
    QImage image;
  }
  upload;

  unsigned int texture_id=0;

};



} //namespace

#endif //__VISUS_GL_TEXTURE_H
