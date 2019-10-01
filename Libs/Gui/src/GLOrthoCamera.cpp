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
GLOrthoCamera::GLOrthoCamera(double default_scale) : default_scale(default_scale)
{
  this->smooth = 0.3;
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
  if (in.name == "Assign")
  {
    this->readFrom(in);
    return;
  }

  if (in.name == "SetLookAt")
  {
    auto pos = Point3d::fromString(in.readString("pos"));
    auto dir = Point3d::fromString(in.readString("dir"));
    auto vup = Point3d::fromString(in.readString("vup"));
    setLookAt(pos, dir, vup);
    return;
  }

  if (in.name == "Translate")
  {
    auto vt = Point2d::fromString(in.readString("value"));
    translate(vt);
    return;
  }

  if (in.name == "Scale")
  {
    auto vs = in.readDouble("vs");
    auto center = Point2d::fromString(in.readString("center"));
    scale(vs, center);
    return;
  }

  if (in.name == "Rotate")
  {
    auto quantity = in.readDouble("value");
    rotate(quantity);
    return;
  }

  if (in.name == "GuessPosition")
  {
    auto p1 = Point3d::fromString(in.readString("p1"));
    auto p2 = Point3d::fromString(in.readString("p2"));
    auto ref = in.readInt("ref");
    guessPosition(BoxNd(p1, p2), ref);
    return;
  }

  if (in.name == "SetProperty")
  {
    auto name = in.readString("name");

    if (name == "ortho_params")
    {
      auto value = GLOrthoParams::fromString(in.readString("value"));
      setOrthoParams(value);
      return;
    }

    if (name == "viewport")
    {
      auto value = Viewport::fromString(in.readString("value"));
      setViewport(value);
      return;
    }

    if (name == "min_zoom")
    {
      auto value = in.readDouble("value");
      setMinZoom(value);
      return;
    }

    if (name == "max_zoom")
    {
      auto value = in.readDouble("value");
      setMaxZoom(value);
      return;
    }

    if (name == "smooth")
    {
      auto value = in.readDouble("value");
      setSmooth(value);
      return;
    }

    if (name == "disable_rotation")
    {
      auto value = in.readDouble("value");
      setDisableRotation(value);
      return;
    }

    ThrowException("internal error");
  }

  return GLCamera::executeAction(in);
}


  //////////////////////////////////////////////////
void GLOrthoCamera::mirror(int ref)
{
  auto value = getOrthoParams();

  if (ref == 0) 
    std::swap(value.left, value.right);

  else if (ref == 1) 
    std::swap(value.top, value.bottom);

  setOrthoParams(value);
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMousePressEvent(QMouseEvent* evt)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(),evt->y());
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glMouseMoveEvent(QMouseEvent* evt)
{
  int button=
    (evt->buttons() & Qt::LeftButton  )? Qt::LeftButton  : 
    (evt->buttons() & Qt::RightButton )? Qt::RightButton : 
    (evt->buttons() & Qt::MiddleButton)? Qt::MiddleButton:
    0;

  if (!button) 
    return ;

  this->mouse.glMouseMoveEvent(evt);

  int W=getViewport().width;
  int H=getViewport().height;

#if 0
  if (
      this->mouse.getButton(Qt::LeftButton).isDown && 
      this->mouse.getButton(Qt::MidButton).isDown && 
      (button==Qt::LeftButton || button==Qt::MidButton))
  {
    //t1 and t2 are the old position, T1 and T2 are the touch position.
    FrustumMap map=needUnprojectInScreenSpace();
    Point2d t1 = map.unprojectPoint(last_mouse_pos[1].castTo<Point2d>()).toPoint2(), T1 = map.unprojectPoint(this->mouse.getButton(Qt::LeftButton).pos.castTo<Point2d>()).toPoint2();
    Point2d t2 = map.unprojectPoint(last_mouse_pos[2].castTo<Point2d>()).toPoint2(), T2 = map.unprojectPoint(this->mouse.getButton(Qt::MidButton ).pos.castTo<Point2d>()).toPoint2();
    Point2d center = (t1 + t2)*0.5;

    //since I'm handling both buttons here, I need to "freeze" the actual situation (otherwise I will replicate the transformation twice)
    last_mouse_pos[1] = this->mouse.getButton(Qt::LeftButton).pos;
    last_mouse_pos[2] = this->mouse.getButton(Qt::MidButton).pos;
    
    /* 
    what I want is as SCALE * TRANSLATE * ROTATE, the solution works only if I set a good center
    The system has 4 unknowns (a b tx ty) and 4 equations

    [t1-center]:=[x1 y1]
    [t2-center]:=[x2 y2]
    [T1-center]:=[x3 y3]
    [T2-center]:=[x4 y4]

    [ a b tx] [x1] = [x3]
    [-b a ty] [y1] = [y3]
    [ 0 0  1] [ 1] = [ 1]

    [ a b tx] [x2] = [x4]
    [-b a ty] [y2] = [y4]
    [ 0 0  1] [ 1] = [1 ] 
    */
    double x1 = t1[0]-center[0], y1=t1[1]-center[1], x2=t2[0]-center[0], y2=t2[1]-center[1];
    double x3 = T1[0]-center[0], y3=T1[1]-center[1], x4=T2[0]-center[0], y4=T2[1]-center[1];
    double D  = ((y1-y2)*(y1-y2) + (x1-x2)*(x1-x2));
    double a  = ((y1-y2)*(y3-y4) + (x1-x2)*(x3-x4))/D;
    double b  = ((y1-y2)*(x3-x4) - (x1-x2)*(y3-y4))/D;
    double tx = (x3-a*x1-b*y1);
    double ty = (y3+b*x1-a*y1);

    //invalid 
    if (D == 0 || a == 0
      || !Utils::isValidNumber(a ) || !Utils::isValidNumber(b )
      || !Utils::isValidNumber(tx) || !Utils::isValidNumber(ty))
    {
      evt->accept();
      return;
    }

    double vs = 1.0/sqrt(a*a + b*b);

    pushAction(???);
    {
      scale(vs, center);
      translate(-Point2d(tx, ty));
      rotate(-atan2(b, a));
    }
    popAction();

    evt->accept();
    return;
  }
#endif

  if (this->mouse.getButton(Qt::LeftButton).isDown && button==Qt::LeftButton)
  {
    FrustumMap map= needUnprojectInScreenSpace();
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
  last_mouse_pos[evt->button()] = Point2i(evt->x(),evt->y());
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glWheelEvent(QWheelEvent* evt)
{
  FrustumMap map= needUnprojectInScreenSpace();
  Point2d center = map.unprojectPoint(Point2d(evt->x(),evt->y())).toPoint2();
  double  vs     = evt->delta()>0 ? (1.0 / default_scale) : (default_scale);
  scale(vs,center);
  evt->accept();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glKeyPressEvent(QKeyEvent* evt)
{
  int key=evt->key();

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

  if (key==Qt::Key_M) 
  {
    toggleSmooth();
    evt->accept();
    return;
  }
}

////////////////////////////////////////////////////////////////
bool GLOrthoCamera::guessPosition(BoxNd bound,int ref)
{
  bound.setPointDim(3);

  int W = getViewport().width ; if (!W) W=800;
  int H = getViewport().height; if (!H) H=800;

  auto size = bound.size();

  pushAction(
    StringTree("GuessPosition").write("p1", bound.p1.toString()).write("p2", bound.p2.toString()).write("ref",ref),
    EncodeObject(this, "Assign"));
  {
    if (ref == 0 || (ref < 0 && !size[0] && size[1] && size[2]))
    {
      this->pos = Point3d(0, 0, 0);
      this->dir = -Point3d(1, 0, 0);
      this->vup = +Point3d(0, 0, 1);
      this->rotation_angle = 0.0;
      double Xnear = -bound.p1[0];
      double Xfar  = -bound.p2[0];
      if (Xnear == Xfar) { Xnear += 1; Xfar -= 1; }
      auto params = GLOrthoParams(bound.p1[1], bound.p2[1], bound.p1[2], bound.p2[2], Xnear, Xfar);
      params.fixAspectRatio((double)W / (double)H);
      setOrthoParams(params);
    }
    else if (ref == 1 || (ref < 0 && size[0] && !size[1] && size[2]))
    {
      this->pos = Point3d(0, 0, 0);
      this->dir = +Point3d(0, 1, 0);
      this->vup = +Point3d(0, 0, 1);
      this->rotation_angle = 0.0;
      double Ynear = +bound.p1[1];
      double Yfar  = +bound.p2[1];
      if (Ynear == Yfar) { Ynear -= 1; Yfar += 1; }
      auto params = GLOrthoParams(bound.p1[0], bound.p2[0], bound.p1[2], bound.p2[2], Ynear, Yfar);
      params.fixAspectRatio((double)W / (double)H);
      setOrthoParams(params);
    }
    else
    {
      VisusAssert(ref < 0 || ref == 2);
      this->pos = Point3d(0, 0, 0);
      this->dir = -Point3d(0, 0, 1);
      this->vup = +Point3d(0, 1, 0);
      this->rotation_angle = 0.0;
      double Znear = -bound.p1[2];
      double Zfar  = -bound.p2[2];
      if (Znear == Zfar) { Znear += 1; Zfar -= 1; }
      auto params = GLOrthoParams(bound.p1[0], bound.p2[0], bound.p1[1], bound.p2[1], Znear, Zfar);
      params.fixAspectRatio((double)W / (double)H);
      setOrthoParams(params);
    }
  }
  popAction();

  return true;
}

//////////////////////////////////////////////////
void GLOrthoCamera::setOrthoParams(GLOrthoParams new_value)
{
  VisusAssert(VisusHasMessageLock());

  timer.reset();

  auto old_value = getOrthoParams();
  if (old_value  == new_value)
    return;

  pushAction(
    StringTree("SetProperty").write("name", "ortho_params").write("value", new_value.toString()),
    EncodeObject(this, "Assign"));
  {
    this->ortho_params_final = new_value;
    this->ortho_params_current = new_value;
  }
  popAction();
}

//////////////////////////////////////////////////
void GLOrthoCamera::setViewport(Viewport new_value)
{
  auto old_value=this->viewport;
  if (old_value == new_value)
    return;

  auto params=getOrthoParams();
  params.fixAspectRatio(old_value, new_value);

  pushAction(
    StringTree("SetProperty").write("name","viewport").write("value",new_value.toString()),
    EncodeObject(this, "Assign"));
  {
    this->viewport = new_value;
    setOrthoParams(params);
  }
  popAction();
}

////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getCurrentFrustum() const
{
  Frustum ret;
  ret.loadProjection(ortho_params_current.getProjectionMatrix(true));
  ret.multProjection(Matrix::rotateAroundCenter(ortho_params_current.getCenter(),Point3d(0,0,1),rotation_angle));
  ret.loadModelview(Matrix::lookAt(pos,pos+dir,vup));
  ret.setViewport(this->viewport);
  return ret;
}

////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getFinalFrustum() const
{
  Frustum ret;
  ret.loadProjection(ortho_params_final.getProjectionMatrix(true));
  ret.multProjection(Matrix::rotateAroundCenter(ortho_params_final.getCenter(), Point3d(0, 0, 1), rotation_angle));
  ret.loadModelview(Matrix::lookAt(pos, pos + dir, vup));
  ret.setViewport(this->viewport);
  return ret;
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::setLookAt(Point3d pos, Point3d dir, Point3d vup)
{
  pushAction(
    StringTree("SetLookAt").write("pos", pos.toString()).write("dir", dir.toString()).write("vup", vup.toString()),
    fullUndo());
  {
    this->pos = pos;
    this->dir = dir;
    this->vup = vup;
  }
  popAction();
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::translate(Point2d vt)
{
  if (vt==Point2d()) return;

  pushAction(
    StringTree("Translate").write("value",vt.toString()),
    fullUndo());
  {
    this->ortho_params_final.translate(Point3d(vt));
    interpolateToFinal();
  }
  popAction();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::rotate(double quantity)
{
  if (!quantity || disable_rotation) return;

  pushAction(
    StringTree("Rotate").write("value", quantity),
    fullUndo());
  {
    this->rotation_angle += quantity;
  }
  popAction();
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::scale(double vs,Point2d center)
{
  if (vs==1 || vs==0) 
    return;

  pushAction(
    StringTree("Scale").write("vs", cstring(vs)).write("center",center.toString()),
    fullUndo());
  {
    this->ortho_params_final.scaleAroundCenter(Point3d(vs, vs, 1.0), Point3d(center));
    interpolateToFinal();
  }
  popAction();
}



////////////////////////////////////////////////////////////////
void GLOrthoCamera::interpolateToFinal()
{
  VisusAssert(VisusHasMessageLock());

  //schedule next updates
  if (smooth > 0 && !timer)
  {
    this->timer = std::make_shared<QTimer>();
    QObject::connect(timer.get(), &QTimer::timeout, [this] {
      VisusInfo()<<"interpolating";
      interpolateToFinal();
    });
    this->timer->setInterval(1000); //as much as possible
    this->timer->start();
  }

  GLOrthoParams value = GLOrthoParams::interpolate(smooth, this->ortho_params_current, 1 - smooth, this->ortho_params_final);

  if (bool bReachedTheEnd = (value == this->ortho_params_current) || (value == this->ortho_params_final))
  {
    value = this->ortho_params_final;
    this->timer.reset();
  }

  //limit the zoom to actual pixels visible 
  if ((max_zoom > 0 || min_zoom > 0) && getViewport().valid())
  {
    Point2d pixel_per_sample(
      (double)getViewport().width / value.getWidth(),
      (double)getViewport().height / value.getHeight());

    if (bool bTooMuchZoom = max_zoom > 0 && std::max(pixel_per_sample[0], pixel_per_sample[1]) > max_zoom)
    {
      this->timer.reset();
      return;
    }

    if (bool bTooLessZoom = (min_zoom > 0 && std::min(pixel_per_sample[0], pixel_per_sample[1]) < min_zoom))
    {
      this->timer.reset();
      return;
    }
  }

  //technically ortho_params_current is not part of the "model" so I'm not
  //communicate the changes to the outside
  //pushAction(...)
  this->ortho_params_current = value;
  this->redisplay_needed.emitSignal();
  //popAction()
}


 ////////////////////////////////////////////////////////////////
void GLOrthoCamera::writeTo(StringTree& out) const
{
  GLCamera::writeTo(out);

  out.writeValue("pos",pos.toString());
  out.writeValue("dir",dir.toString());
  out.writeValue("vup",vup.toString());

  out.writeValue("ortho_params", ortho_params_final.toString());
  out.writeValue("rotation_angle", cstring(rotation_angle));

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

  this->ortho_params_final = GLOrthoParams::fromString(in.readValue("ortho_params"));
  this->rotation_angle = cdouble(in.readValue("rotation_angle"));

  this->default_scale=cdouble(in.readValue("default_scale"));
  this->disable_rotation=cbool(in.readValue("disable_rotation"));
  this->max_zoom=cdouble(in.readValue("max_zoom"));
  this->min_zoom=cdouble(in.readValue("min_zoom"));
  this->smooth=cdouble(in.readValue("smooth"));

  this->ortho_params_current = this->ortho_params_final;

}
  
} //namespace

