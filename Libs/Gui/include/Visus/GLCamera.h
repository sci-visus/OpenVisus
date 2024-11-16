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

#ifndef VISUS_GL_CAMERA_H
#define VISUS_GL_CAMERA_H

#include <Visus/Gui.h>
#include <Visus/Frustum.h>
#include <Visus/GLObject.h>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLCamera : public Model
{
public:

  VISUS_NON_COPYABLE_CLASS(GLCamera)

  //a signal needed when the GLCamera model does not change, but display params do
  //(example: see GLOrthoCamera::refineToFinal)
#if !SWIG
  Signal<void()> redisplay_needed;
#endif

  //constructor
  GLCamera() {
  }

  //destructor
  virtual ~GLCamera() {
  }

  //decode
  static SharedPtr<GLCamera> decode(StringTree& ar);

  //getLookAt
  virtual void getLookAt(Point3d& pos, Point3d& center, Point3d& vup) const =0 ;

  //getTypeName
  virtual String getTypeName() const override = 0;

  //mirror
  virtual void mirror(int ref) {
  }

  //guessPosition
  virtual bool guessPosition(BoxNd box, int ref = -1) = 0;

  //splitFrustum
  virtual void splitFrustum(Rectangle2d value)=0;

  //glRender
  virtual void glRender(GLCanvas& gl);

public:

  //glMousePressEvent
  virtual void glMousePressEvent(QMouseEvent* evt, const Viewport& viewport) = 0;

  //glMouseMoveEvent
  virtual void glMouseMoveEvent(QMouseEvent* evt, const Viewport& viewport) = 0;

  //glMouseReleaseEvent
  virtual void glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport) = 0;

  //glWheelEvent
  virtual void glWheelEvent(QWheelEvent* evt, const Viewport& viewport) = 0;

  //keyboard glKeyPressEvent
  virtual void glKeyPressEvent(QKeyEvent* evt, const Viewport& viewport) = 0;

  //getCurrentFrustum
  virtual Frustum getCurrentFrustum(const Viewport& viewport) const = 0;

  //getFinalFrustum
  virtual Frustum getFinalFrustum(const Viewport& viewport) const = 0;

public:

  //beginTransaction (needed by swig)
  void beginTransaction() {
    Model::beginTransaction();
  }

  //endTransaction (needed by swig)
  void endTransaction() {
    Model::endTransaction();
  }


public:

  //write
  virtual void write(Archive& ar) const  override {
  }

  //read
  virtual void read(Archive& ar) override  {
  }

protected:

  //modelChanged
  virtual void modelChanged() override {
    redisplay_needed.emitSignal();
  }

};//end class


} //namespace Visus

#endif //VISUS_GL_CAMERA_H

