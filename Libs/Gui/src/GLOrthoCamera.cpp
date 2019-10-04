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
}

//////////////////////////////////////////////////
GLOrthoCamera::~GLOrthoCamera() {
  VisusAssert(VisusHasMessageLock());
  timer.reset();
}

  //////////////////////////////////////////////////
void GLOrthoCamera::executeAction(StringTree in) 
{
  if (in.name == "setLookAt")
  {
    auto pos = Point3d::fromString(in.readString("pos"));
    auto dir = Point3d::fromString(in.readString("dir"));
    auto vup = Point3d::fromString(in.readString("vup"));
    auto rotation = cdouble(in.readString("rotation"));
    setLookAt(pos, dir, vup, rotation);
    return;
  }

  if (in.name == "set")
  {
    auto target_id = in.readString("target_id");

    if (target_id == "ortho_params")
    {
      auto value = GLOrthoParams::fromString(in.readString("value"));
      auto smooth = in.readDouble("smooth");
      setOrthoParams(value,smooth);
      return;
    }

    if (target_id == "viewport")
    {
      auto value = Viewport::fromString(in.readString("value"));
      setViewport(value);
      return;
    }

    if (target_id == "min_zoom")
    {
      auto value = in.readDouble("value");
      setMinZoom(value);
      return;
    }

    if (target_id == "max_zoom")
    {
      auto value = in.readDouble("value");
      setMaxZoom(value);
      return;
    }

    if (target_id == "smooth")
    {
      auto value = in.readDouble("value");
      setSmooth(value);
      return;
    }

    if (target_id == "disable_rotation")
    {
      auto value = in.readDouble("value");
      setDisableRotation(value);
      return;
    }
  }

  return GLCamera::executeAction(in);
}


  //////////////////////////////////////////////////
void GLOrthoCamera::mirror(int ref)
{
  auto params = getOrthoParams();

  if (ref == 0) 
    std::swap(params.left, params.right);

  else if (ref == 1) 
    std::swap(params.top, params.bottom);

  setOrthoParams(params, /*smooth*/0.0);
}

////////////////////////////////////////////////////////////////
bool GLOrthoCamera::guessPosition(BoxNd bounds,int ref)
{
  bounds.setPointDim(3);

  int W = getViewport().width ; if (!W) W=800;
  int H = getViewport().height; if (!H) H=800;

  auto size = bounds.size();

  Point3d pos(0, 0, 0), dir, vup;
  GLOrthoParams params;

  if (ref == 0 || (ref < 0 && !size[0] && size[1] && size[2]))
  {
    dir = Point3d(-1, 0, 0);
    vup = Point3d( 0, 0, 1);
    params = GLOrthoParams(bounds.p1[1], bounds.p2[1], bounds.p1[2], bounds.p2[2], -bounds.p1[0], -bounds.p2[0]);
  }
  else if (ref == 1 || (ref < 0 && size[0] && !size[1] && size[2]))
  {
    dir = Point3d(0, -1, 0);
    vup = Point3d(0,  0, 1);
    params = GLOrthoParams(bounds.p1[0], bounds.p2[0], bounds.p1[2], bounds.p2[2], -bounds.p1[1], -bounds.p2[1]);
  }
  else
  {
    VisusAssert(ref < 0 || ref == 2);
    dir = Point3d(0, 0, -1);
    vup = Point3d(0, 1,  0);
    params = GLOrthoParams(bounds.p1[0], bounds.p2[0], bounds.p1[1], bounds.p2[1], -bounds.p1[2], -bounds.p2[2]);
  }

  if (params.zNear == params.zFar) { 
    auto Z = params.zNear;
    params.zNear = Z + 1;
    params.zFar  = Z - 1;
  }

  beginUpdate(Transaction(),Transaction());
  {
    setLookAt(pos, dir, vup, /*rotation*/0.0);
    setOrthoParams(params.withAspectRatio(W, H), /*smooth*/ 0.0);
  }
  endUpdate();

  return true;
}

//////////////////////////////////////////////////
void GLOrthoCamera::setOrthoParams(GLOrthoParams new_value,double smooth)
{
  VisusAssert(VisusHasMessageLock());

  new_value = checkZoomRange(new_value);

  auto old_value = getOrthoParams();
  if (old_value  == new_value)
    return;

  beginUpdate(
    StringTree("set").write("target_id", "ortho_params").write("value", new_value.toString()).write("smooth", smooth),
    StringTree("set").write("target_id", "ortho_params").write("value", old_value.toString()).write("smooth", smooth));
  {
    this->ortho_params_final   = checkZoomRange(new_value);
  }
  endUpdate();


  //technically ortho_params_current is not part of the "model" so I'm not
  //communicate the changes to the outside
  if (smooth)
  {
    if (!timer)
    {
      this->timer = std::make_shared<QTimer>();
      this->timer->setInterval(/*msec*/10);
      QObject::connect(timer.get(), &QTimer::timeout, [this, smooth] 
      {
        this->ortho_params_current = GLOrthoParams::interpolate(smooth, this->ortho_params_current, 1 - smooth, this->ortho_params_final);

        //reached the end?
        if (this->ortho_params_current == this->ortho_params_final)
          this->timer.reset();

        this->redisplay_needed.emitSignal();
      });
      this->timer->start();
    }
  }
  else
  {
    this->ortho_params_current = this->ortho_params_final;
    timer.reset();
    this->redisplay_needed.emitSignal();
  }
}

//////////////////////////////////////////////////
void GLOrthoCamera::setViewport(Viewport new_value)
{
  auto old_value=this->viewport;
  if (old_value == new_value)
    return;

  //try to keep the ortho params (if possible)
  auto params=getOrthoParams().withAspectRatio(old_value, new_value);

  beginUpdate(
    StringTree("set").write("target_id", "viewport").write("value", new_value.toString()),
    StringTree("set").write("target_id", "viewport").write("value", old_value.toString()));
  {
    this->viewport = new_value;
    setOrthoParams(params, /*smoth*/0.0);
  }
  endUpdate();
}

////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getCurrentFrustum() const
{
  Frustum ret;

  ret.setViewport(this->viewport);

  ret.loadProjection(ortho_params_current.getProjectionMatrix(true));

  if (this->rotation)
    ret.multProjection(Matrix::rotateAroundCenter(ortho_params_current.getCenter(),Point3d(0,0,1),rotation));

  ret.loadModelview(Matrix::lookAt(pos,pos+dir,vup));

  return ret;
}

////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getFinalFrustum() const
{
  Frustum ret;
  ret.loadProjection(ortho_params_final.getProjectionMatrix(true));

  if (this->rotation)
    ret.multProjection(Matrix::rotateAroundCenter(ortho_params_final.getCenter(), Point3d(0, 0, 1), rotation));

  ret.loadModelview(Matrix::lookAt(pos, pos + dir, vup));
  ret.setViewport(this->viewport);
  return ret;
}

////////////////////////////////////////////////////////////////
GLOrthoParams GLOrthoCamera::checkZoomRange(GLOrthoParams value) const
{
  //limit the zoom to actual pixels visible 
  if (!getViewport().valid())
    return value;

  // viewport_w/ortho_params_w=zoom 
  // min_zoom<=zoom<=max_zoom
  // min_zoom <= viewport_w/ortho_params_w <= max_zoom
  // ortho_params_w>= viewport_w/max_zoom
  // ortho_params_w<= viewport_w/min_zoom

  double zoom = std::max(
    (double)getViewport().width / value.getWidth(),
    (double)getViewport().height / value.getHeight());

  //example max_zoom=2.0 means value.getWidth can be 0.5*viewport_w
  if (max_zoom > 0 && zoom > max_zoom)
  {
    auto ortho_params_w = (double)getViewport().width / max_zoom;
    auto ortho_params_h = (double)getViewport().height / max_zoom;

    value = GLOrthoParams(
      value.getCenter().x - 0.5 * ortho_params_w, value.getCenter().x + 0.5 * ortho_params_w,
      value.getCenter().y - 0.5 * ortho_params_h, value.getCenter().y + 0.5 * ortho_params_h,
      value.zNear, value.zFar).withAspectRatio((double)getViewport().width, (double)getViewport().height);
  }

  if (min_zoom > 0 && zoom < min_zoom)
  {
    auto ortho_params_w = (double)getViewport().width / min_zoom;
    auto ortho_params_h = (double)getViewport().height / min_zoom;

    value = GLOrthoParams(
      value.getCenter().x - 0.5 * ortho_params_w, value.getCenter().x + 0.5 * ortho_params_w,
      value.getCenter().y - 0.5 * ortho_params_h, value.getCenter().y + 0.5 * ortho_params_h,
      value.zNear, value.zFar).withAspectRatio((double)getViewport().width, (double)getViewport().height);
  }

  return value;
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::setLookAt(Point3d pos, Point3d dir, Point3d vup, double rotation)
{
  beginUpdate(
    StringTree("setLookAt").write("pos", pos.toString()).write("dir", dir.toString()).write("vup", vup.toString()).write("rotation", rotation),
    StringTree("setLookAt").write("pos", this->pos.toString()).write("dir", this->dir.toString()).write("vup", this->vup.toString()).write("rotation", this->rotation));
  {
    this->pos = pos;
    this->dir = dir;
    this->vup = vup;
    this->rotation = rotation;
  }
  endUpdate();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::translate(Point2d vt)
{
  if (vt == Point2d()) return;
  auto params = this->ortho_params_final;
  params.translate(Point3d(vt));
  setOrthoParams(params, this->smooth);
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::scale(double vs,Point2d center)
{
  if (vs==1 || vs==0)  return;
  auto params = this->ortho_params_final;
  params.scaleAroundCenter(Point3d(vs, vs, 1.0), Point3d(center));
  setOrthoParams(params, this->smooth);
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMousePressEvent(QMouseEvent* evt)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(), evt->y());
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMouseMoveEvent(QMouseEvent* evt)
{
  int button =
    (evt->buttons() & Qt::LeftButton) ? Qt::LeftButton :
    (evt->buttons() & Qt::RightButton) ? Qt::RightButton :
    (evt->buttons() & Qt::MiddleButton) ? Qt::MiddleButton :
    0;

  if (!button)
    return;

  this->mouse.glMouseMoveEvent(evt);

  int W = getViewport().width;
  int H = getViewport().height;

  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton)
  {
    FrustumMap map = needUnprojectInScreenSpace();
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
void GLOrthoCamera::glMouseReleaseEvent(QMouseEvent* evt)
{
  this->mouse.glMouseReleaseEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(), evt->y());
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glWheelEvent(QWheelEvent* evt)
{
  FrustumMap map = needUnprojectInScreenSpace();
  Point2d center = map.unprojectPoint(Point2d(evt->x(), evt->y())).toPoint2();
  double  vs = evt->delta() > 0 ? (1.0 / default_scale) : (default_scale);
  scale(vs, center);
  evt->accept();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glKeyPressEvent(QKeyEvent* evt)
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
    toggleSmooth();
    evt->accept();
    return;
  }
}


 ////////////////////////////////////////////////////////////////
void GLOrthoCamera::writeTo(StringTree& out) const
{
  GLCamera::writeTo(out);

  out.writeValue("pos",pos.toString());
  out.writeValue("dir",dir.toString());
  out.writeValue("vup",vup.toString());
  out.writeValue("rotation", cstring(rotation));

  out.writeValue("ortho_params", ortho_params_final.toString());

  out.writeValue("default_scale", cstring(default_scale));
  out.writeValue("disable_rotation", cstring(disable_rotation));
  out.writeValue("max_zoom", cstring(max_zoom));
  out.writeValue("min_zoom", cstring(min_zoom));
  out.writeValue("smooth", cstring(smooth));
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::readFrom(StringTree& in) 
{
  GLCamera::readFrom(in);

  this->pos=Point3d::fromString(in.readValue("pos","0  0  0"));
  this->dir=Point3d::fromString(in.readValue("dir","0  0 -1"));
  this->vup=Point3d::fromString(in.readValue("vup","0  1  0"));
  this->rotation = cdouble(in.readValue("rotation"));

  this->ortho_params_final = GLOrthoParams::fromString(in.readValue("ortho_params"));

  this->default_scale=cdouble(in.readValue("default_scale"));
  this->disable_rotation=cbool(in.readValue("disable_rotation"));
  this->max_zoom=cdouble(in.readValue("max_zoom"));
  this->min_zoom=cdouble(in.readValue("min_zoom"));
  this->smooth=cdouble(in.readValue("smooth"));

  this->ortho_params_current = this->ortho_params_final;

}
  
} //namespace

