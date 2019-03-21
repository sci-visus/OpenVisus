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

#ifndef VISUS_GL_ORTHO_CAMERA_H
#define VISUS_GL_ORTHO_CAMERA_H

#include <Visus/GLCamera.h>
#include <Visus/GLMouse.h>

#include <QTimer>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLOrthoCamera : public GLCamera
{
public:

  VISUS_NON_COPYABLE_CLASS(GLOrthoCamera)

  //constructor
  GLOrthoCamera(double default_scale = 1.3) : default_scale(default_scale) 
  {
    last_mouse_pos.resize(GLMouse::getNumberOfButtons());
    QObject::connect(&timer,&QTimer::timeout,[this]{
      refineToFinal();
    });
    timer.setInterval(0);
  }

  //destructor
  virtual ~GLOrthoCamera() {
    VisusAssert(VisusHasMessageLock());
    timer.stop();
  }

  //getTypeName
  virtual String getTypeName() const override {
    return "GLOrthoCamera";
  }

  //getLookAt
  void getLookAt(Point3d& pos, Point3d& dir, Point3d& vup) const {
    pos = this->pos;
    dir = this->dir;
    vup = this->vup;
  }

  //setLookAt
  void setLookAt(Point3d pos, Point3d dir, Point3d vup)
  {
    beginUpdate();
    this->pos = pos;
    this->dir = dir;
    this->vup = vup;
    endUpdate();
  }

  //guessPosition
  virtual bool guessPosition(Position position, int ref = -1) override;

  //isRotationDisabled
  bool isRotationDisabled() const {
    return bDisableRotation;
  }

  //setRotationDisabled
  void setRotationDisabled(bool value) {
    if (bDisableRotation==value) return;
    beginUpdate();
    bDisableRotation=value;
    endUpdate();
  }

  //getMaxZoom
  double getMaxZoom() const {
    return max_zoom;
  }

  //setMaxZoom
  void setMaxZoom(double value) {
    if (max_zoom==value) return;
    beginUpdate();
    max_zoom=value;
    endUpdate();
  }
  

  //getMinZoom
  double getMinZoom() const {
    return min_zoom;
  }

  //setMinZoom
  void setMinZoom(double value) {
    if (min_zoom==value) return;
    beginUpdate();
    min_zoom=value;
    endUpdate();
  }

  //mirror
  virtual void mirror(int ref) override;

  //scale
  void scale(double vs,Point2d center);

  //scale
  void scale(Point2d center) {
    scale(default_scale,center);
  }

  //rotate
  void rotate(double quantity);

  //getOrthoParams
  virtual GLOrthoParams getOrthoParams() const override {
     return ortho_params; 
  }

  //setOrthoParams
  virtual void setOrthoParams(GLOrthoParams value) override;

  //getViewport
  virtual Viewport getViewport() const override {
    return getFrustum().getViewport();
  }

  //setViewport
  virtual void setViewport(Viewport value) override;

  //getFrustum
  virtual Frustum getFrustum() const override {
    return getFrustum(ortho_params);
  }

  //getFinalFrustum
  virtual Frustum getFinalFrustum() const override {
    return getFrustum(ortho_params_final);
  }

  //glMousePressEvent
  virtual void glMousePressEvent(QMouseEvent* evt) override;

  //glMouseReleaseEvent
  virtual void glMouseReleaseEvent(QMouseEvent* evt) override;

  //glMouseMoveEvent
  virtual void glMouseMoveEvent(QMouseEvent* evt) override;

  //glWheelEvent
  virtual void glWheelEvent(QWheelEvent* evt) override;

  //glKeyPressEvent
  virtual void glKeyPressEvent(QKeyEvent* evt) override;


public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  double                smooth=0.90;
  bool                  bDisableRotation = false;
  double                default_scale;
  double                rotation_angle = 0;
  double                max_zoom = 0;
  double                min_zoom = 0;
  GLMouse               mouse;
  std::vector<Point2i>  last_mouse_pos;
  Viewport              viewport;

  Point3d               pos = Point3d(0, 0, 0);
  Point3d               dir = Point3d(0, 0,-1);
  Point3d               vup = Point3d(0, 1, 0);

  GLOrthoParams         ortho_params;
  GLOrthoParams         ortho_params_final;

  QTimer                timer;

  //translate
  void translate(Point2d vt);

  //needUnproject
  FrustumMap needUnproject()
  {
    Frustum temp(getFrustum());
    temp.loadModelview(Matrix::identity()); // i don't want the modelview...
    return FrustumMap(temp);
  }


  //getFrustum
  Frustum getFrustum(GLOrthoParams ortho_params) const ;

  //refineToFinal
  void refineToFinal();

}; //end class


} //namespace Visus

#endif //VISUS_GL_ORTHO_CAMERA_H

