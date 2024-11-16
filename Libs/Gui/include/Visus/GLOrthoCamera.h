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
#include <Visus/GLOrthoParams.h>
#include <Visus/GLMouse.h>

#include <QTimer>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLOrthoCamera : public GLCamera
{
public:

  VISUS_NON_COPYABLE_CLASS(GLOrthoCamera)

  //constructor
  GLOrthoCamera(double default_scale = 1.3);

  //destructor
  virtual ~GLOrthoCamera();

  //getTypeName
  virtual String getTypeName() const override {
    return "GLOrthoCamera";
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
  void setLookAt(Point3d pos, Point3d center, Point3d vup, double rotation = 0.0);

  //guessPosition
  virtual bool guessPosition(BoxNd bound, int ref = -1) override;

  //splitFrustum
  virtual void splitFrustum(Rectangle2d r) override;

  //setDisableRotation
  void setDisableRotation(bool value) {
    setProperty("SetDisableRotation", this->disable_rotation, value);
  }

  //isRotationDisabled
  bool isRotationDisabled() const {
    return disable_rotation;
  }

  //getMaxZoom
  double getMaxZoom() const {
    return max_zoom;
  }

  //setMaxZoom
  void setMaxZoom(double value) {
    setProperty("SetMaxZoom", this->max_zoom, value);
  }
  
  //getMinZoom
  double getMinZoom() const {
    return min_zoom;
  }

  //setMinZoom
  void setMinZoom(double value) {
    setProperty("SetMinZoom", this->min_zoom, value);
  }

  //mirror
  virtual void mirror(int ref) override;

  //translate
  void translate(Point2d vt);

  //moveLeft
  void moveLeft() {
    translate(Point2d(-getOrthoParams().getWidth(), 0.0));
  }

  //moveRight
  void moveRight() {
    translate(Point2d(+getOrthoParams().getWidth(), 0));
  }

  //moveUp
  void moveUp() {
    translate(Point2d(0, +getOrthoParams().getHeight()));
  }

  //moveDown
  void moveDown() {
    translate(Point2d(0, -getOrthoParams().getHeight()));
  }

  //scale
  void scale(double vs,Point2d center);

  //scale
  void scale(Point2d center) {
    scale(default_scale,center);
  }

  //scale
  void scale(double vs) {
    scale(vs, getOrthoParams().getCenter().toPoint2());
  }

  //zoomIn
  void zoomIn() {
    scale(1.0 / default_scale);
  }

  //scale(Qt::Key_Plus ? 1.0 / default_scale : default_scale);
  void zoomOut() {
    scale(default_scale);
  }

  //getOrthoParams
  GLOrthoParams getOrthoParams() const  {
     return ortho_params.current; 
  }

  //setOrthoParams
  void setOrthoParams(GLOrthoParams value, int smooth=0);

  //getDefaultSmooth
  int getDefaultSmooth() const {
    return default_smooth;
  }

  //setDefaultSmooth
  void setDefaultSmooth(int value) {
    setProperty("SetDefaultSmooth", this->default_smooth, value);
  }

  //toggleDefaultSmooth
  void toggleDefaultSmooth() {
    setDefaultSmooth(getDefaultSmooth() ? 0 : 1300);
  }

public:

  //getCurrentFrustum
  virtual Frustum getCurrentFrustum(const Viewport& viewport) const override;

  //getFinalFrustum
  virtual Frustum getFinalFrustum(const Viewport& viewport) const override;

  //glMousePressEvent
  virtual void glMousePressEvent(QMouseEvent* evt, const Viewport& viewport) override;

  //glMouseReleaseEvent
  virtual void glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport) override;

  //glMouseMoveEvent
  virtual void glMouseMoveEvent(QMouseEvent* evt, const Viewport& viewport) override;

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

  bool                  disable_rotation = false;
  double                default_scale;
  double                max_zoom = 0;
  double                min_zoom = 0;
  GLMouse               mouse;
  std::vector<Point2i>  last_mouse_pos;

  Point3d               pos    = Point3d(0, 0,  0);
  Point3d               center = Point3d(0, 0, -1);
  Point3d               vup    = Point3d(0, 1,  0);
  double                rotation = 0.0;

  //interpolate ortho params
  struct
  {
    QTimer             timer;
    Time               t1;
    GLOrthoParams      initial;
    GLOrthoParams      current;
    GLOrthoParams      final;
    int                msec;      
  }
  ortho_params;

  int default_smooth;

  //refineToFinal
  void refineToFinal();

  //needUnproject
  FrustumMap needUnprojectInScreenSpace(const Viewport& viewport)
  {
    Frustum temp(getCurrentFrustum(viewport));
    temp.loadModelview(Matrix::identity(4)); // i don't want the modelview...
    return FrustumMap(temp);
  }

  //checkZoomRange
  GLOrthoParams checkZoomRange(GLOrthoParams value, const Viewport& viewport) const;

}; //end class


} //namespace Visus

#endif //VISUS_GL_ORTHO_CAMERA_H

