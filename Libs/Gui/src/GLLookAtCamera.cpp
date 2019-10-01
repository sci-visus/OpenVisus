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

#include <Visus/GLLookAtCamera.h>
#include <Visus/Log.h>

namespace Visus {

//////////////////////////////////////////////////
void GLLookAtCamera::executeAction(StringTree in)
{
  if (in.name == "Assign")
  {
    this->readFrom(in);
    return;
  }

  if (in.name == "SetLookAt")
  {
    auto pos = Point3d::fromString(in.readString("pos"));
    auto center = Point3d::fromString(in.readString("center"));
    auto vup = Point3d::fromString(in.readString("vup"));
    setLookAt(pos, center, vup);
    return;
  }

  if (in.name == "MoveInWorld")
  {
    auto vt = Point3d::fromString(in.readString("vt"));
    moveInWorld(vt);
    return;
  }

  if (in.name == "RotateAroundScreenCenter")
  {
    auto angle = in.readDouble("angle");
    auto screen_center = Point2d::fromString(in.readString("screen_center"));
    rotateAroundScreenCenter(angle, screen_center);
    return;
  }

  if (in.name == "RotateAroundWorldAxis")
  {
    auto angle = in.readDouble("angle");
    auto axis = Point3d::fromString(in.readString("axis"));
    rotateAroundWorldAxis(angle, axis);
    return;
  }

  if (in.name == "SetProperty")
  {
    auto name = in.read("name");

    if (name == "viewport")
    {
      auto value = Viewport::fromString(in.read("value"));
      setViewport(value);
      return;
    }

    if (name == "center_of_rotation")
    {
      auto value = Point3d::fromString(in.read("value"));
      setCenterOfRotation(value);
      return;
    }

    if (name == "use_ortho_projection")
    {
      auto value = in.readBool("value");
      setUseOrthoProjection(value);
      return;
    }

    if (name == "ortho_params")
    {
      auto value = GLOrthoParams::fromString(in.read("value"));
      setOrthoParams(value);
      return;
    }

    if (name == "ortho_params_fixed")
    {
      auto value = in.readBool("value");
      setOrthoParamsFixed(value);
      return;
    }

    if (name == "bounds")
    {
      auto p1 = Point3d::fromString(in.readString("p1"));
      auto p2 = Point3d::fromString(in.readString("p2"));
      auto value = BoxNd(p1, p2);
      setBounds(value);
      return;
    }

    ThrowException("internal error");
  }

  return GLCamera::executeAction(in);
}

//////////////////////////////////////////////////
void GLLookAtCamera::setViewport(Viewport new_value)
{
  auto old_value=getViewport();
  if (old_value == new_value)
    return;

  pushAction(
    StringTree("SetProperty").write("name","viewport").write("value", new_value.toString()),
    fullUndo());
  {
    this->viewport = new_value;
    guessOrthoParams();
  }
  popAction();
}



//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setCenterOfRotation(Point3d new_value)
{
  auto old_value = this->center_of_rotation;
  if (new_value == old_value)
    return;

  pushAction(
    StringTree("SetProperty").write("name","center_of_rotation").write("value", new_value.toString()),
    fullUndo());
  {
    this->center_of_rotation = new_value;
    this->dir = (this->center_of_rotation - this->pos).normalized();
    guessOrthoParams();
  }
  popAction();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setUseOrthoProjection(bool new_value)
{
  auto old_value = this->use_ortho_projection;
  if (new_value== old_value)
    return;
 
  pushAction(
    StringTree("SetProperty").write("name","use_ortho_projection").write("value", cstring(new_value)),
    fullUndo());
  {
    this->use_ortho_projection = new_value;
    guessOrthoParams();
  }
  popAction();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setOrthoParams(GLOrthoParams new_value) 
{
  auto old_value = getOrthoParams();
  if (old_value == new_value)
    return;

  pushAction(
    StringTree("SetProperty").write("name","ortho_params").write("value", new_value.toString()),
    fullUndo());
  {
    this->ortho_params = new_value;
  }
  popAction();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setOrthoParamsFixed(bool new_value) 
{
  auto old_value = this->ortho_params_fixed;
  if (new_value ==old_value)
    return;
 
  pushAction(
    StringTree("SetProperty").write("name","ortho_params_fixed").write("value", cstring(new_value)),
    fullUndo());
  {
    this->ortho_params_fixed = new_value;
    guessOrthoParams();
  }
  popAction();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMousePressEvent(QMouseEvent* evt)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(),evt->y());
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseMoveEvent(QMouseEvent* evt)
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

  // 2-finger zoom/pan/rotate
#if 0
  if (
    this->mouse.getButton(Qt::LeftButton).isDown && 
    this->mouse.getButton(Qt::MidButton).isDown && 
    (button==Qt::LeftButton || button==Qt::MidButton)) 
  {
    //t1 and t2 are the old position, T1 and T2 are the touch position.
    Point2d t1 = last_mouse_pos[1].castTo<Point2d>(), T1 = this->mouse.getButton(Qt::LeftButton).pos.castTo<Point2d>();
    Point2d t2 = last_mouse_pos[2].castTo<Point2d>(), T2 = this->mouse.getButton(Qt::MidButton ).pos.castTo<Point2d>();
    Point2d center=(t1+t2)*0.5;

    /* what I want is as SCALE * TRANSLATE * ROTATE, the solution works only if I set a good center
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
      [ 0 0  1] [ 1] = [1 ] */

    double x1 = t1[0]-center[0], y1=t1[1]-center[1], x2=t2[0]-center[0], y2=t2[1]-center[1];
    double x3 = T1[0]-center[0], y3=T1[1]-center[1], x4=T2[0]-center[0], y4=T2[1]-center[1];
    double D  = ((y1-y2)*(y1-y2) + (x1-x2)*(x1-x2));
    double a  = ((y1-y2)*(y3-y4) + (x1-x2)*(x3-x4))/D;
    double b  = ((y1-y2)*(x3-x4) - (x1-x2)*(y3-y4))/D;
    double tx = (x3-a*x1-b*y1);
    double ty = (y3+b*x1-a*y1);

    double scale = sqrt(a*a + b*b);
    double angle = -1.0*atan2(-b,a);

    //invalid
    if (D==0 || !scale || !Utils::isValidNumber(scale)  || !Utils::isValidNumber(angle) || !Utils::isValidNumber(tx) || !Utils::isValidNumber(ty))
    {
      evt->accept();
      return;
    }

    //since I'm handling both buttons here, I need to "freeze" the actual situation (otherwise I will replicate the transformation twice)
    last_mouse_pos[1] = this->mouse.getButton(Qt::LeftButton).pos;
    last_mouse_pos[2] = this->mouse.getButton(Qt::MidButton).pos;

    pushAction(???);
    {
      rotateAroundScreenCenter(angle,center);

      Point3d vt;
      vt.x = (-tx / getViewport().width ) * guessForwardFactor() * pan_factor;
      vt.y = (-ty / getViewport().height) * guessForwardFactor() * pan_factor;
      vt.z = (a < 1 ? -1 : +1)            * guessForwardFactor();
      moveInWorld(vt);
    }
    popAction();

    evt->accept();
    return;
  }
#endif

  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton) 
  {
    Point2i screen_p1 = last_mouse_pos[button];
    Point2i screen_p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = screen_p2;

    double R = ortho_params.getWidth() / 2.0;

    // project the center of rotation to the screen
    Frustum temp;
    temp.setViewport(getViewport());
    temp.loadProjection(Matrix::perspective(60, W / (double)H, ortho_params.zNear, ortho_params.zFar));
    temp.loadModelview(Matrix::lookAt(this->pos, this->pos + this->dir, this->vup));

    FrustumMap map(temp);
    Point3d center = map.unprojectPointToEye(map.projectPoint(center_of_rotation));
    Point3d p1 = map.unprojectPointToEye(screen_p1.castTo<Point2d>());
    Point3d p2 = map.unprojectPointToEye(screen_p2.castTo<Point2d>());

    double x1 = p1[0] - center[0], y1 = p1[1] - center[1];
    double x2 = p2[0] - center[0], y2 = p2[1] - center[1];
    double l1 = x1 * x1 + y1 * y1, l2 = x2 * x2 + y2 * y2;
    if (l1 < R * R / 2)
      p1[2] = sqrt(R * R - l1);
    else
      p1[2] = R * R / 2 / sqrt(l1);

    if (l2 < R * R / 2)
      p2[2] = sqrt(R * R - l2);
    else
      p2[2] = R * R / 2 / sqrt(l2);

    auto M = Matrix::lookAt(this->pos, this->pos + this->dir, this->vup).invert();
    Point3d a = (M * p1 - M * center).normalized();
    Point3d b = (M * p2 - M * center).normalized();
    Point3d axis = a.cross(b).normalized();
    double angle = acos(a.dot(b));

    if (!axis.module() || !axis.valid())
    {
      evt->accept();
      return;
    }

    rotateAroundWorldAxis(angle, axis);

    evt->accept();
    return;
  }

  // pan
  if (this->mouse.getButton(Qt::RightButton).isDown && button==Qt::RightButton) 
  {
    Point2i p1 = last_mouse_pos[button];
    Point2i p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = p2;

    Point3d vt;
    vt.x = ((p1.x - p2.x) / getViewport().width ) * guessForwardFactor() * pan_factor;
    vt.y = ((p1.y - p2.y) / getViewport().height) * guessForwardFactor() * pan_factor;
    vt.z = 0.0;
    moveInWorld(vt);
    evt->accept();
    return;
  }

  last_mouse_pos[button] = Point2i(evt->x(),evt->y());
}



//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseReleaseEvent(QMouseEvent* evt)
{
  this->mouse.glMouseReleaseEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(),evt->y());
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glWheelEvent(QWheelEvent* evt)
{
  Point3d vt;
  vt.x = 0.0;
  vt.y = 0.0;
  vt.z = (evt->delta() < 0 ? -1 : +1) * guessForwardFactor();
  moveInWorld(vt);
  evt->accept();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glKeyPressEvent(QKeyEvent* evt) 
{
  int key=evt->key();

  //moving the projection
  if  (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down ) 
  {
    double dx=0,dy=0;
    if (key==Qt::Key_Left ) dx=-ortho_params.getWidth();
    if (key==Qt::Key_Right) dx=+ortho_params.getWidth();
    if (key==Qt::Key_Up   ) dy=+ortho_params.getHeight();
    if (key==Qt::Key_Down ) dy=-ortho_params.getHeight();

    auto ortho_params=this->ortho_params;
    ortho_params.translate(Point3d(dx,dy));
    setOrthoParams(ortho_params);
    evt->accept(); 
    return; 
  }

  switch(key)
  {
    // TODO: Use double-tap to accomplish same thing on iPhone.
    case Qt::Key_X: // recenter the camera
    {
      // TODO: No access to scene, so just set center of rotation
      // at current distance, center of screen. Would like to
      // intersect ray with bounding volumes of scene components and
      // use an actual point on/in one of these volumes, but all we
      // have is the global bound. Sometimes this is acceptable
      // (e.g. a section of neural microscopy), but it's not always
      // sufficient (e.g. large volume/more complicated scene).

      // TODO: really want to use mouse position for center, not just center of screen.
      int W=getViewport().width/2;
      int H=getViewport().height/2;
      auto ray=FrustumMap(getCurrentFrustum()).getRay(Point2d(W,H));

      RayBoxIntersection intersection(ray,bounds);

      if (intersection.valid)
        setCenterOfRotation(ray.getPoint(0.5*(std::max(intersection.tmin,0.0)+intersection.tmax)).toPoint3());

      evt->accept();
      return ;
    }
    case Qt::Key_P:
    {
      VisusInfo() << EncodeObject(this, getTypeName()).toString();
      evt->accept();
      return;
    }

    case Qt::Key_M:
    {
      setUseOrthoProjection(!this->use_ortho_projection);
      evt->accept();
      return;
    }
  }
}


//////////////////////////////////////////////////////////////////////
bool GLLookAtCamera::guessPosition(BoxNd value,int ref) 
{
  value.setPointDim(3);

  pushAction(
    StringTree("GuessPosition").write("bound", value.toString()).write("ref",cstring(ref)),
    fullUndo());
  {
    this->bounds= value;

    this->use_ortho_projection=false;
    this->center_of_rotation= value.center().toPoint3();

    if (ref<0)
    {
      this->pos = center_of_rotation +2.1* value.size().toPoint3();
      this->dir =(center_of_rotation-pos).normalized();
      this->vup = Point3d(0,0,1);
    }
    else
    {
      const std::vector<Point3d> Axis = {Point3d(1,0,0),Point3d(0,1,0),Point3d(0,0,1)};
      this->pos=center_of_rotation + 2.1f * value.maxsize()*Axis[ref];
      this->dir=-Axis[ref];
      this->vup = std::vector<Point3d>({Point3d(0,0,1),Point3d(0,0,1),Point3d(0,1,0)})[ref];
    }

    //at the beginning no rotation
    this->quaternion= Quaternion();

    guessOrthoParams();
  }
  popAction();

  return true;
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setBounds(BoxNd new_value)
{
  new_value.setPointDim(3);

  auto old_value = this->bounds;
  if (old_value == new_value)
    return;

  pushAction(
    StringTree("SetProperty").write("name","bounds").write("p1", new_value.p1.toPoint3().toString()).write("p2", new_value.p2.toPoint3().toString()),
    fullUndo());
  {
    this->bounds = new_value;
    this->center_of_rotation = new_value.center().toPoint3();
  }
  popAction();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setLookAt(Point3d pos, Point3d center, Point3d vup)
{
  pushAction(
    StringTree("SetLookAt").write("pos", pos.toString()).write("center", center.toString()).write("vup", vup.toString()),
    fullUndo());
  {
    this->pos=pos;
    this->dir= (center - pos).normalized();
    this->vup= vup;
    this->center_of_rotation = center;
    this->quaternion= Quaternion();
    guessOrthoParams();
  }
  popAction();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::moveInWorld(Point3d vt)
{
  auto dir = this->dir;
  auto right = this->dir.cross(vup).normalized();
  auto up = right.cross(dir);

  pushAction(
    StringTree("MoveInWorld").write("vt", vt.toString()),
    fullUndo());
  {
    if (right.valid())
      this->pos += vt.x * right;

    if (up.valid())
      this->pos += vt.y * up;

    if (dir.valid())
      this->pos += vt.z * dir;

    guessOrthoParams();
  }
  popAction();
}



//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::rotateAroundScreenCenter(double angle, Point2d screen_center)
{
  if (this->disable_rotation || !angle) 
    return ;

  pushAction(
    StringTree("RotateAroundScreenCenter").write("angle",angle).write("screen_center",screen_center.toString()),
    fullUndo());
  {
    int W = getViewport().width;
    int H = getViewport().height;

    FrustumMap map(getCurrentFrustum());
    Point3d world_center = map.unprojectPoint(screen_center);
    auto Trot = Matrix::translate(+1 * world_center) * Matrix::rotateAroundAxis(dir, angle) * Matrix::translate(-1 * world_center);
    auto mv = Matrix::lookAt(this->pos, this->pos + this->dir, this->vup) * Trot;
    Point3d POS, DIR, VUP; mv.getLookAt(POS, DIR, VUP);
    if (!POS.valid() || !DIR.valid() || !VUP.valid() || (POS == pos && DIR == dir && VUP == vup)) return;

    this->pos = POS;
    this->dir = DIR;
    this->vup = VUP;

    guessOrthoParams();
  }
  popAction();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::rotateAroundWorldAxis(double angle, Point3d axis)
{
  if (!angle)
    return;

  pushAction(
    StringTree("RotateAroundWorldAxis").write("angle", angle).write("axis", axis.toString()),
    fullUndo());
  {
    this->quaternion = Quaternion(axis, angle * rotation_factor) * this->quaternion;
    guessOrthoParams();
  }
  popAction();
}

//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getFinalFrustum() const
{
  Frustum ret;
  ret.setViewport(viewport);
  ret.loadModelview(Matrix::lookAt(this->pos,this->pos+this->dir,this->vup));
  ret.multModelview(Matrix::rotateAroundCenter(center_of_rotation,quaternion.getAxis(),quaternion.getAngle()));
  ret.loadProjection(ortho_params.getProjectionMatrix(use_ortho_projection));
  return ret;
}

//////////////////////////////////////////////////////////////////////
std::pair<double,double> GLLookAtCamera::guessNearFarDistance() const 
{
  Point3d far_point ((this->dir[0] >= 0) ? bounds.p2[0] : bounds.p1[0], (this->dir[1] >= 0) ? bounds.p2[1] : bounds.p1[1], (this->dir[2] >= 0) ? bounds.p2[2] : bounds.p1[2]);
  Point3d near_point((this->dir[0] >= 0) ? bounds.p1[0] : bounds.p2[0], (this->dir[1] >= 0) ? bounds.p1[1] : bounds.p2[1], (this->dir[2] >= 0) ? bounds.p1[2] : bounds.p2[2]);

  Plane camera_plane(this->dir, this->pos);
  double near_dist = camera_plane.getDistance(near_point);
  double far_dist  = camera_plane.getDistance(far_point);

  return std::make_pair(near_dist,far_dist);
}

//////////////////////////////////////////////////////////////////////
double GLLookAtCamera::guessForwardFactor() const 
{
  auto   pair     =guessNearFarDistance();
  double near_dist=pair.first;
  double far_dist =pair.second;

  // update the sensitivity of moving forward

  // when camera inside volume
  // roughly means 64 scroll steps to go through the volume
  if (near_dist <= 0) 
    return (far_dist - near_dist) / 64; 

  // if the camera is far away from the volume, it moves toward the volume faster
  else 
    return far_dist / 16; 
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::guessOrthoParams()
{
  if (ortho_params_fixed)
    return;

  auto old_value = this->ortho_params;

  auto   pair = guessNearFarDistance();
  double near_dist = pair.first;
  double far_dist = pair.second;

  double ratio = getViewport().width / (double)getViewport().height;
  const double fov = 60.0;

  auto new_value = GLOrthoParams();
  new_value.zNear = near_dist <= 0 ? 0.1 : near_dist;
  new_value.zFar = far_dist + far_dist - near_dist;
  new_value.top = new_value.zNear * tan(fov * Math::Pi / 360.0);
  new_value.bottom = -new_value.top;
  new_value.left = new_value.bottom * ratio;
  new_value.right = new_value.top * ratio;

  setOrthoParams(new_value);
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::writeTo(StringTree& out) const
{
  GLCamera::writeTo(out);

  out.writeValue("bound", bounds.toString(/*bInterleave*/false));

  out.writeValue("pos", pos.toString());
  out.writeValue("dir", dir.toString());
  out.writeValue("vup", vup.toString());

  out.writeValue("quaternion", quaternion.toString());
  out.writeValue("center_of_rotation", center_of_rotation.toString());

  out.writeValue("ortho_params", ortho_params.toString());
  out.writeValue("ortho_params_fixed", cstring(ortho_params_fixed));

  out.writeValue("disable_rotation", cstring(disable_rotation));
  out.writeValue("use_ortho_projection", cstring(use_ortho_projection));
  out.writeValue("rotation_factor", cstring(rotation_factor));
  out.writeValue("pan_factor", cstring(pan_factor));
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::readFrom(StringTree& in) 
{
  GLCamera::readFrom(in);

  this->bounds = BoxNd::fromString(in.readValue("bound"),/*bInterleave*/false);
  this->bounds.setPointDim(3);

  this->pos = Point3d::fromString(in.readValue("pos"));
  this->dir = Point3d::fromString(in.readValue("dir"));
  this->vup = Point3d::fromString(in.readValue("vup"));

  this->quaternion = Quaternion::fromString(in.readValue("quaternion"));
  this->center_of_rotation = Point3d::fromString(in.readValue("center_of_rotation"));

  this->ortho_params = GLOrthoParams::fromString(in.readValue("ortho_params", this->ortho_params.toString()));
  this->ortho_params_fixed = in.readBool("ortho_params_fixed", this->ortho_params_fixed);
  
  this->disable_rotation = in.readBool("disable_rotation",this->disable_rotation);
  this->use_ortho_projection = in.readBool("use_ortho_projection",this->use_ortho_projection);
  this->rotation_factor = in.readDouble("rotation_factor", this->rotation_factor);
  this->pan_factor = in.readDouble("pan_factor", this->pan_factor);

}

} //namespace Visus
