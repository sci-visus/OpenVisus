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

#include <Visus/GLOrthoCamera.h>
#include <Visus/LocalCoordinateSystem.h>

namespace Visus {

 //////////////////////////////////////////////////
GLOrthoCamera::GLOrthoCamera(double default_scale) 
  : default_scale(default_scale)
{
  last_mouse_pos.resize(GLMouse::getNumberOfButtons());

  //this means that during mouse interaction it will take <n> msec to move from one place to another
  this->default_smooth = 500;

  ortho_params.timer.setInterval(10);
  QObject::connect(&ortho_params.timer, &QTimer::timeout, [this]() {
    refineToFinal();
  });
}

//////////////////////////////////////////////////
GLOrthoCamera::~GLOrthoCamera() {
  VisusAssert(VisusHasMessageLock());
  ortho_params.timer.stop();
}

  //////////////////////////////////////////////////
void GLOrthoCamera::execute(Archive& ar) 
{
  if (ar.name == "SetOrthoParams")
  {
    GLOrthoParams value; int smooth;
    ar.read("value", value);
    ar.read("smooth", smooth, false);
    setOrthoParams(value, smooth);
    return;
  }

  if (ar.name == "SetMinZoom")
  {
    double value;
    ar.read("value", value);
    setMinZoom(value);
    return;
  }

  if (ar.name == "SetMaxZoom")
  {
    double value;
    ar.read("value", value);
    setMaxZoom(value);
    return;
  }

  if (ar.name == "SetDefaultSmooth")
  {
    int value;
    ar.read("value", value, 1300);
    setDefaultSmooth(value);
    return;
  }

  if (ar.name == "SetDisableRotation")
  {
    bool value;
    ar.read("value", value);
    setDisableRotation(value);
    return;
  }

  if (ar.name == "SetLookAt")
  {
    Point3d pos, center, vup; double rotation;
    ar.read("pos", pos);
    ar.read("center", center);
    ar.read("vup", vup);
    ar.read("rotation", rotation, 0.0);
    setLookAt(pos, center, vup, rotation);
    return;
  }

  return GLCamera::execute(ar);
}


  //////////////////////////////////////////////////
void GLOrthoCamera::mirror(int ref)
{
  auto params = getOrthoParams();

  if (ref == 0) 
    std::swap(params.left, params.right);

  else if (ref == 1) 
    std::swap(params.top, params.bottom);

  setOrthoParams(params);
}

////////////////////////////////////////////////////////////////
bool GLOrthoCamera::guessPosition(BoxNd bounds,int ref)
{
  bounds.setPointDim(3);

  auto size = bounds.size();

  Point3d pos(0, 0, 0), center, vup;
  GLOrthoParams params;

  if (ref == 0 || (ref < 0 && !size[0] && size[1] && size[2]))
  {
    center = Point3d(-1, 0, 0);
    vup = Point3d( 0, 0, 1);
    params = GLOrthoParams(bounds.p1[1], bounds.p2[1], bounds.p1[2], bounds.p2[2], -bounds.p1[0], -bounds.p2[0]);
  }
  else if (ref == 1 || (ref < 0 && size[0] && !size[1] && size[2]))
  {
    center = Point3d(0, -1, 0);
    vup = Point3d(0,  0, 1);
    params = GLOrthoParams(bounds.p1[0], bounds.p2[0], bounds.p1[2], bounds.p2[2], -bounds.p1[1], -bounds.p2[1]);
  }
  else
  {
    VisusAssert(ref < 0 || ref == 2);
    center = Point3d(0, 0, -1);
    vup = Point3d(0, 1,  0);
    params = GLOrthoParams(bounds.p1[0], bounds.p2[0], bounds.p1[1], bounds.p2[1], -bounds.p1[2], -bounds.p2[2]);
  }

  if (params.zNear == params.zFar) { 
    auto Z = params.zNear;
    params.zNear = Z + 1;
    params.zFar  = Z - 1;
  }

  const double aspect_ratio = 4.0 / 3.0;

  beginTransaction();
  {
    setLookAt(pos, center, vup, /*rotation*/0.0);
    setOrthoParams(params.withAspectRatio(aspect_ratio));
  }
  endTransaction();

  return true;
}

//////////////////////////////////////////////////
void GLOrthoCamera::splitFrustum(Rectangle2d r)
{
  GLOrthoParams params = this->ortho_params.final; 
  setOrthoParams(params.split(r));
}

//////////////////////////////////////////////////
void GLOrthoCamera::refineToFinal()
{
  auto A = ortho_params.initial;
  auto B = ortho_params.final;
  auto alpha = Utils::clamp(ortho_params.t1.elapsedMsec() / double(ortho_params.msec), 0.0, 1.0);

  auto value = A + (B - A) * pow(alpha, 1 / 2.0);

  if (value == B || value == this->ortho_params.current)
    value = B;

  this->ortho_params.current = value;

  //reached the end?
  if (value == B)
    ortho_params.timer.stop();

  this->redisplay_needed.emitSignal();
}

//////////////////////////////////////////////////
void GLOrthoCamera::setOrthoParams(GLOrthoParams new_value, int smooth)
{
  VisusAssert(VisusHasMessageLock());

  auto& old_value = this->ortho_params.final;
  if (old_value == new_value)
    return;

  beginUpdate(
    StringTree("SetOrthoParams").write("value", new_value).write("smooth", smooth), 
    StringTree("SetOrthoParams").write("value", old_value).write("smooth", smooth));
  {
    old_value = new_value;
  }
  endUpdate();

  //technically ortho_params.current is not part of the "model" so I'm not
  //communicate the changes to the outside
  if (smooth)
  {
    if (!ortho_params.timer.isActive())
    {
      ortho_params.initial = ortho_params.current;
      ortho_params.t1 = Time::now();
      ortho_params.msec = smooth;
      ortho_params.timer.start();
    }
  }
  else
  {
    this->ortho_params.current = this->ortho_params.final;
    ortho_params.timer.stop();
    this->redisplay_needed.emitSignal();
  }
}


////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getCurrentFrustum(const Viewport& viewport) const
{
  Frustum ret;
  ret.setViewport(viewport);

  auto params = (this->ortho_params.current).withAspectRatio(viewport.getAspectRatio());
  ret.loadProjection(Matrix::ortho(params.left, params.right, params.bottom, params.top, params.zNear, params.zFar));

  if (this->rotation)
    ret.multProjection(Matrix::rotateAroundCenter(params.getCenter(),Point3d(0,0,1),rotation));

  ret.loadModelview(Matrix::lookAt(pos,center,vup));

  return ret;
}

////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getFinalFrustum(const Viewport& viewport) const
{
  Frustum ret;
  ret.setViewport(viewport);

  auto params = (this->ortho_params.final).withAspectRatio(viewport.getAspectRatio());
  ret.loadProjection(Matrix::ortho(params.left, params.right, params.bottom, params.top, params.zNear, params.zFar));

  if (this->rotation)
    ret.multProjection(Matrix::rotateAroundCenter(ortho_params.final.getCenter(), Point3d(0, 0, 1), rotation));

  ret.loadModelview(Matrix::lookAt(pos, center, vup));
  return ret;
}

////////////////////////////////////////////////////////////////
GLOrthoParams GLOrthoCamera::checkZoomRange(GLOrthoParams value, const Viewport& viewport) const
{
  //limit the zoom to actual pixels visible 
  if (!viewport.valid())
    return value;

  // viewport_w/ortho_params_w=zoom 
  // min_zoom<=zoom<=max_zoom
  // min_zoom <= viewport_w/ortho_params_w <= max_zoom
  // ortho_params_w>= viewport_w/max_zoom
  // ortho_params_w<= viewport_w/min_zoom

  double zoom = std::max(
    (double)viewport.width / value.getWidth(),
    (double)viewport.height / value.getHeight());

  //example max_zoom=2.0 means value.getWidth can be 0.5*viewport_w
  if (max_zoom > 0 && zoom > max_zoom)
  {
    auto ortho_params_w = (double)viewport.width / max_zoom;
    auto ortho_params_h = (double)viewport.height / max_zoom;

    value = GLOrthoParams(
      value.getCenter().x - 0.5 * ortho_params_w, value.getCenter().x + 0.5 * ortho_params_w,
      value.getCenter().y - 0.5 * ortho_params_h, value.getCenter().y + 0.5 * ortho_params_h,
      value.zNear, value.zFar).withAspectRatio((double)viewport.width / (double)viewport.height);
  }

  if (min_zoom > 0 && zoom < min_zoom)
  {
    auto ortho_params_w = (double)viewport.width / min_zoom;
    auto ortho_params_h = (double)viewport.height / min_zoom;

    value = GLOrthoParams(
      value.getCenter().x - 0.5 * ortho_params_w, value.getCenter().x + 0.5 * ortho_params_w,
      value.getCenter().y - 0.5 * ortho_params_h, value.getCenter().y + 0.5 * ortho_params_h,
      value.zNear, value.zFar).withAspectRatio((double)viewport.width / (double)viewport.height);
  }

  return value;
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::setLookAt(Point3d pos, Point3d center, Point3d vup, double rotation)
{
  beginUpdate(
    StringTree("SetLookAt").write("pos",       pos.toString()).write("center",       center.toString()).write("vup",       vup.toString()).write("rotation",       rotation),
    StringTree("SetLookAt").write("pos", this->pos.toString()).write("center", this->center.toString()).write("vup", this->vup.toString()).write("rotation", this->rotation));
  {
    this->pos = pos;
    this->center = center;
    this->vup = vup;
    this->rotation = rotation;
  }
  endUpdate();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::translate(Point2d vt)
{
  if (vt == Point2d()) return;
  setOrthoParams(this->ortho_params.final.translated(Point3d(vt)), getDefaultSmooth());
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::scale(double vs,Point2d center)
{
  if (vs==1 || vs==0)  return;
  setOrthoParams(this->ortho_params.final.scaledAroundCenter(Point3d(vs, vs, 1.0), Point3d(center)), getDefaultSmooth());
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMousePressEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(), evt->y());
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMouseMoveEvent(QMouseEvent* evt, const Viewport& viewport)
{
  int button =
    (evt->buttons() & Qt::LeftButton) ? Qt::LeftButton :
    (evt->buttons() & Qt::RightButton) ? Qt::RightButton :
    (evt->buttons() & Qt::MiddleButton) ? Qt::MiddleButton :
    0;

  if (!button)
    return;

  this->mouse.glMouseMoveEvent(evt);

  int W = viewport.width;
  int H = viewport.height;

  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton)
  {
    FrustumMap map = needUnprojectInScreenSpace(viewport);
    Point2i p1 = last_mouse_pos[button];
    Point2i p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = p2;
    Point2d t1 = map.unprojectPoint(p1.castTo<Point2d>()).toPoint2();
    Point2d T1 = map.unprojectPoint(p2.castTo<Point2d>()).toPoint2();
    translate(t1 - T1);
    evt->accept();
    return;
  }
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMouseReleaseEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(), evt->y());
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glWheelEvent(QWheelEvent* evt, const Viewport& viewport)
{
  FrustumMap map = needUnprojectInScreenSpace(viewport);
  Point2d center = map.unprojectPoint(Point2d(evt->x(), evt->y())).toPoint2();
  double  vs = evt->delta() > 0 ? (1.0 / default_scale) : (default_scale);
  if (!vs || vs == 1) return;
  auto params = this->ortho_params.final;
  params = params.scaledAroundCenter(Point3d(vs, vs, 1.0), Point3d(center));
  params = checkZoomRange(params, viewport);
  setOrthoParams(params, getDefaultSmooth());
  evt->accept();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glKeyPressEvent(QKeyEvent* evt, const Viewport& viewport)
{
  int key = evt->key();

  if (key == Qt::Key_Left)
  {
    moveLeft();
    evt->accept();
    return;
  }

  if (key == Qt::Key_Right)
  {
    moveRight();
    evt->accept();
    return;
  }

  if (key == Qt::Key_Up)
  {
    moveUp();
    evt->accept();
    return;
  }


  if (key == Qt::Key_Down)
  {
    moveDown();
    evt->accept();
    return;
  }

  if (key == Qt::Key_Plus)
  {
    zoomIn();
    evt->accept();
    return;
  }

  if (key == Qt::Key_Minus)
  {
    zoomOut();
    evt->accept();
    return;
  }

  if (key == Qt::Key_M)
  {
    toggleDefaultSmooth();
    evt->accept();
    return;
  }
}


 ////////////////////////////////////////////////////////////////
void GLOrthoCamera::write(Archive& ar) const
{
  GLCamera::write(ar);

  ar.write("pos", pos);
  ar.write("center", center);
  ar.write("vup", vup);
  ar.write("rotation", rotation);
  ar.write("ortho_params", ortho_params.final);
  ar.write("default_scale", default_scale);
  ar.write("disable_rotation", disable_rotation);
  ar.write("max_zoom", max_zoom);
  ar.write("min_zoom", min_zoom);
  ar.write("default_smooth", default_smooth);
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::read(Archive& ar)
{
  GLCamera::read(ar);

  ar.read("pos", pos);
  ar.read("center", center);
  ar.read("vup", vup);
  ar.read("rotation", rotation, this->rotation);
  ar.read("ortho_params", ortho_params.final);
  ar.read("default_scale", default_scale, this->default_scale);
  ar.read("disable_rotation", disable_rotation, this->disable_rotation);
  ar.read("max_zoom", max_zoom, this->max_zoom);
  ar.read("min_zoom", min_zoom, this->min_zoom);
  ar.read("default_smooth", default_smooth, this->default_smooth);

  this->ortho_params.current = this->ortho_params.final;

}
  
} //namespace

