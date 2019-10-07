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
  if (in.name == "set")
  {
    auto target_id = in.read("target_id");

    if (target_id == "bounds") {
      setBounds(BoxNd::fromString(in.readString("value")));
      return;
    }

    if (target_id == "pos") {
      setPosition(Point3d::fromString(in.readString("value")));
      return;
    }

    if (target_id == "dir") {
      setDirection(Point3d::fromString(in.readString("value")));
      return;
    }

    if (target_id == "vup") {
      setViewUp(Point3d::fromString(in.readString("value")));
      return;
    }

    if (target_id == "rotation")
    {
      auto angle = Utils::degreeToRadiant(in.readDouble("angle"));
      auto axis = Point3d::fromString(in.readString("axis"));
      setRotation(Quaternion(axis,angle));
      return;
    }

    if (target_id == "rotation_center") {
      setRotationCenter(Point3d::fromString(in.readString("value")));
      return;
    }

    if (target_id == "fov") {
      setFov(in.readDouble("fov"));
      return;
    }

    if (target_id == "split_projection_frustum") {
      splitProjectionFrustum(Rectangle2d::fromString(in.readString("value")));
      return;
    }

  }

  return GLCamera::executeAction(in);
}

//////////////////////////////////////////////////////////////////////
std::pair<double,double> GLLookAtCamera::guessNearFar() const
{
  auto dir = this->dir.normalized();
  Point3d far_point ((dir[0] >= 0) ? bounds.p2[0] : bounds.p1[0], (dir[1] >= 0) ? bounds.p2[1] : bounds.p1[1], (dir[2] >= 0) ? bounds.p2[2] : bounds.p1[2]);
  Point3d near_point((dir[0] >= 0) ? bounds.p1[0] : bounds.p2[0], (dir[1] >= 0) ? bounds.p1[1] : bounds.p2[1], (dir[2] >= 0) ? bounds.p1[2] : bounds.p2[2]);
  Plane camera_plane(dir, this->pos);
  auto zNear = camera_plane.getDistance(near_point);
  auto zFar = camera_plane.getDistance(far_point);
  return std::make_pair(zNear, zFar);
}

//////////////////////////////////////////////////////////////////////
bool GLLookAtCamera::guessPosition(BoxNd bounds, int ref)
{
  bounds.setPointDim(3);

  Point3d pos, dir, vup;
  Point3d center = bounds.center().toPoint3();

  if (ref < 0)
  {
    pos = center + 2.1 * bounds.size().toPoint3();
    dir = (center - pos).normalized();
    vup = Point3d(0, 0, 1);
  }
  else
  {
    const std::vector<Point3d> Axis = { Point3d(1,0,0),Point3d(0,1,0),Point3d(0,0,1) };
    pos = center + 2.1f * bounds.maxsize() * Axis[ref];
    dir = -Axis[ref];
    vup = std::vector<Point3d>({ Point3d(0,0,1),Point3d(0,0,1),Point3d(0,1,0) })[ref];
  }

  beginTransaction();
  {
    setBounds(bounds);
    setPosition(pos);
    setDirection(dir);
    setViewUp(vup);
    setRotation(Quaternion());
    setRotationCenter(center);
    setFov(60.0);
  }
  endTransaction();

  return true;
}


//////////////////////////////////////////////////////////////////////
double GLLookAtCamera::guessForwardFactor() const
{
  // when camera inside volume
  // roughly means 64 scroll steps to go through the volume
  // if the camera is far away from the volume, it moves toward the volume faster
  auto p = guessNearFar();
  auto zNear = p.first;
  auto zFar = p.second;
  return (zNear <= 0)? (zFar - zNear) / 64 : (zFar / 16);
}

//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getFinalFrustum(const Viewport& viewport) const
{
  Frustum ret;
  ret.setViewport(viewport);

  ret.loadModelview(Matrix::lookAt(this->pos, this->pos + this->dir, this->vup));

  if (rotation.getAngle())
    ret.multModelview(Matrix::rotateAroundCenter(this->rotation_center, rotation.getAxis(), rotation.getAngle()));

  auto aspect_ratio = viewport.getAspectRatio();

  auto p = guessNearFar();
  auto zNear = p.first, zFar = p.second;
  if (split_projection_frustum == Rectangle2d(0, 0, 1, 1))
  {
    ret.loadProjection(Matrix::perspective(fov, aspect_ratio, zNear, zFar));
  }
  else
  {
    GLOrthoParams params;
    params.top    = +zNear * tan(Utils::degreeToRadiant(fov * 0.5));
    params.bottom = -zNear * tan(Utils::degreeToRadiant(fov * 0.5));
    params.right  = +params.top * aspect_ratio;
    params.left   = -params.top * aspect_ratio;
    params.zNear  = zNear;
    params.zFar   = zFar;
    params = params.split(split_projection_frustum);
    ret.loadProjection(Matrix::frustum(params.left, params.right, params.bottom, params.top, params.zNear, params.zFar));
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMousePressEvent(QMouseEvent* evt,const Viewport& viewport)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(),evt->y());
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseMoveEvent(QMouseEvent* evt, const Viewport& viewport)
{
  int button=
    (evt->buttons() & Qt::LeftButton  )? Qt::LeftButton  : 
    (evt->buttons() & Qt::RightButton )? Qt::RightButton : 
    (evt->buttons() & Qt::MiddleButton)? Qt::MiddleButton:
    0;

  if (!button) 
    return ;

  this->mouse.glMouseMoveEvent(evt);

  int W= viewport.width;
  int H= viewport.height;

  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton) 
  {
    Point2i screen_p1 = last_mouse_pos[button];
    Point2i screen_p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = screen_p2;

    double R = viewport.width / 2.0;

    auto p = guessNearFar();
    auto zNear = p.first, zFar = p.second;

    // project the center of rotation to the screen
    Frustum temp;
    temp.setViewport(viewport);
    temp.loadProjection(Matrix::perspective(fov, W / (double)H, zNear, zFar));
    temp.loadModelview(Matrix::lookAt(this->pos, this->pos + this->dir, this->vup));

    FrustumMap map(temp);
    Point3d center = map.unprojectPointToEye(map.projectPoint(this->pos+this->dir));
    Point3d p1 = map.unprojectPointToEye(screen_p1.castTo<Point2d>());
    Point3d p2 = map.unprojectPointToEye(screen_p2.castTo<Point2d>());

    double x1 = p1[0] - center[0], y1 = p1[1] - center[1]; double l1 = x1 * x1 + y1 * y1;
    double x2 = p2[0] - center[0], y2 = p2[1] - center[1]; double l2 = x2 * x2 + y2 * y2;
    p1[2] = (l1 < R * R / 2) ? sqrt(R * R - l1) : R * R / 2 / sqrt(l1);
    p2[2] = (l2 < R * R / 2) ? sqrt(R * R - l2) : R * R / 2 / sqrt(l2);

    auto M = Matrix::lookAt(this->pos, this->pos+this->dir, this->vup).invert();
    Point3d a = (M * p1 - M * center).normalized();
    Point3d b = (M * p2 - M * center).normalized();

    auto axis = a.cross(b).normalized();
    auto angle = acos(a.dot(b));

    if (!axis.module() || !axis.valid())
    {
      evt->accept();
      return;
    }

    setRotation(Quaternion(axis, angle * rotation_factor) * getRotation());
    evt->accept();
    return;
  }

  // pan
  if (this->mouse.getButton(Qt::RightButton).isDown && button==Qt::RightButton) 
  {
    Point2i p1 = last_mouse_pos[button];
    Point2i p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = p2;


    auto dir = this->dir.normalized();
    auto right = dir.cross(vup).normalized();
    auto up = right.cross(dir);

    auto dx = (((p1.x - p2.x) / viewport.width ) * pan_factor * guessForwardFactor()) * right;
    auto dy = (((p1.y - p2.y) / viewport.height) * pan_factor * guessForwardFactor()) * up;
    setPosition(this->pos + dx  + dy );

    evt->accept();
    return;
  }

  last_mouse_pos[button] = Point2i(evt->x(),evt->y());
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMouseReleaseEvent(evt);
  evt->accept();
  last_mouse_pos[evt->button()] = Point2i(evt->x(),evt->y());
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glWheelEvent(QWheelEvent* evt, const Viewport& viewport)
{
  auto vt = (evt->delta() < 0 ? -1 : +1) * guessForwardFactor() * this->dir;
  setPosition(this->pos + vt);
  evt->accept();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glKeyPressEvent(QKeyEvent* evt, const Viewport& viewport)
{
  int key=evt->key();

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
      int W= viewport.width/2;
      int H= viewport.height/2;
      auto ray=FrustumMap(getCurrentFrustum(viewport)).getRay(Point2d(W,H));

      RayBoxIntersection intersection(ray,this->bounds);
      if (intersection.valid)
        setRotationCenter(ray.getPoint(0.5 * (std::max(intersection.tmin, 0.0) + intersection.tmax)).toPoint3());

      evt->accept();
      return ;
    }
    case Qt::Key_P:
    {
      VisusInfo() << EncodeObject(*this).toString();
      evt->accept();
      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::writeTo(StringTree& out) const
{
  GLCamera::writeTo(out);

  out.writeValue("bounds", bounds.toString(/*bInterleave*/false));

  out.writeValue("pos",    pos.toString());
  out.writeValue("dir", dir.toString());
  out.writeValue("vup",    vup.toString());

  out.writeValue("rotation", rotation.toString());
  out.writeValue("rotation_center", rotation_center.toString());

  out.writeValue("fov", cstring(this->fov));

  if (split_projection_frustum!=Rectangle2d(0,0,1,1))
    out.writeValue("split_projection_frustum", this->split_projection_frustum.toString());
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::readFrom(StringTree& in) 
{
  GLCamera::readFrom(in);

  this->bounds = BoxNd::fromString(in.readValue("bounds"),/*bInterleave*/false);
  this->bounds.setPointDim(3);

  this->pos = Point3d::fromString(in.readValue("pos"));
  this->dir = Point3d::fromString(in.readValue("dir"));
  this->vup = Point3d::fromString(in.readValue("vup"));

  this->rotation = Quaternion::fromString(in.readValue("rotation"));
  this->rotation_center = Point3d::fromString(in.readValue("rotation_center"));

  this->fov = cdouble(in.readValue("fov"));

  auto s = in.readValue("split_projection_frustum");
  if (!s.empty())
    this->split_projection_frustum = Rectangle2d::fromString(s);
}

} //namespace Visus
