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

#ifndef VISUS_GL_LOOKAT_CAMERA_H
#define VISUS_GL_LOOKAT_CAMERA_H

#include <Visus/GLCamera.h>
#include <Visus/GLOrthoParams.h>
#include <Visus/GLMouse.h>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLookAtCamera : public GLCamera
{
public:

  VISUS_NON_COPYABLE_CLASS(GLLookAtCamera)

  //constructor
  GLLookAtCamera();

  //destructor
  virtual ~GLLookAtCamera();


  //getTypeName
  virtual String getTypeName() const override {
    return "GLLookAtCamera";
  }

  //guessPosition
  virtual bool guessPosition(BoxNd value,int ref=-1) override;

  //splitFrustum
  virtual void splitFrustum(Rectangle2d value) override {
    setProperty("SplitFrustum", this->split_frustum, value);
  }

  //setPos
  void setPos(Point3d value) {
    setProperty("SetPos", this->pos, value);
  }

  //setCenter
  void setCenter(Point3d value) {
    setProperty("SetCenter", this->center, value);
  }

  //setVup
  void setVup(Point3d value) {
    setProperty("SetVup", this->vup, value);
  }

  //getCenterOfRotation
  Point3d getCenterOfRotation() const {
    return center_of_rotation;
  }

  //setCenterOfRotation
  void setCenterOfRotation(Point3d value) {
    setProperty("SetCenterOfRotation", this->center_of_rotation, value);
  }

  //setRotation
  void setRotation(Quaternion value) {
    setProperty("SetRotation",this->rotation, value);
  }

  //getRotation
  Quaternion getRotation() const {
    return rotation;
  }

  //getFov
  double getFov() const {
    return fov;
  }

  //setFov
  void setFov(double value) {
    setProperty("SetFov", this->fov, value);
  }

  //getZNear
  double getZNear() const {
    return zNear;
  }

  //setZNear
  void setZNear(double value) {
    setProperty("SetZNear", this->zNear, value);
  }

  //getZFar
  double getZFar() const {
    return zFar;
  }

  //setZNear
  void setZFar(double value) {
    setProperty("SetZFar", this->zFar, value);
  }

  //getPos
  Point3d getPos() const {
    return pos;
  }

  //getCenter
  Point3d getCenter() const {
    return center;
  }

  //getVup
  Point3d getVup() const {
    return vup;
  }

  //getLookAt
  virtual void getLookAt(Point3d& pos, Point3d& center, Point3d& vup) const override {
    pos = this->pos;
    center = this->center;
    vup = this->vup;
  }

  //setLookAt
  void setLookAt(Point3d pos, Point3d center, Point3d vup);

public:

  //getProjection
  Matrix getProjection(const Viewport& viewport) const;

  //getModelview
  Matrix getModelview() const;

  //getCurrentFrustum
  virtual Frustum getFinalFrustum(const Viewport& viewport) const override
  {
      Frustum ret;
      ret.setViewport(viewport);
      ret.loadProjection(getProjection(viewport));
      ret.loadModelview(getModelview());
      return ret;
  }

  //getCurrentFrustum
  virtual Frustum getCurrentFrustum(const Viewport& viewport) const override {
    return getFinalFrustum(viewport);
  }

public:

  //glMousePressEvent
  virtual void glMousePressEvent(QMouseEvent* evt, const Viewport& viewport) override;

  //glMouseMoveEvent
  virtual void glMouseMoveEvent(QMouseEvent* evt, const Viewport& viewport) override;

  //glMouseReleaseEvent
  virtual void glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport) override;

  //glWheelEvent
  virtual void glWheelEvent(QWheelEvent* evt, const Viewport& viewport) override;

  //glKeyPressEvent
  virtual void glKeyPressEvent(QKeyEvent* evt, const Viewport& viewport) override;


public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

  //projection matrix
  double fov = 60;;
  double zNear = 0;
  double zFar = 0;

  //modelview matrix
  Point3d pos, center, vup;
  Point3d center_of_rotation;
  Quaternion rotation;

  Rectangle2d split_frustum = Rectangle2d(0, 0, 1, 1);

  //run-time
  GLMouse mouse;

};//end class

} //namespace Visus

#endif //VISUS_GL_LOOKAT_CAMERA_H

