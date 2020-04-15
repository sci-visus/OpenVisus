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

namespace Visus {

//////////////////////////////////////////////////
void GLLookAtCamera::execute(Archive& ar)
{
  if (ar.name == "SetBounds") {
    BoxNd value;
    ar.read("value", value);
    setBounds(value);
    return;
  }

  if (ar.name == "SetPosition") {
    Point3d value;
    ar.read("value", value);
    setPosition(value);
    return;
  }

  if (ar.name == "SetDirection") {
    Point3d value;
    ar.read("value", value);
    setDirection(value);
    return;
  }

  if (ar.name == "SetViewUp") {
    Point3d value;
    ar.read("value", value);
    setViewUp(value);
    return;
  }

  if (ar.name == "SetLookAt") {
    Point3d pos,dir,vup;
    ar.read("pos", pos);
    ar.read("dir", dir);
    ar.read("vup", vup);
    setLookAt(pos,dir,vup);
    return;
  }

  if (ar.name == "Rotate") {
    double angle_degree;
    Point3d p0, p1;
    ar.read("angle", angle_degree);
    ar.read("p0", p0);
    ar.read("p1", p1);
    rotate(angle_degree, p0, p1);
    return;
  }

  if (ar.name == "SetFov") {
    double value;
    ar.read("value", value, 60.0);
    setFov(value);
    return;
  }

  if (ar.name == "SplitFrustum") {
    Rectangle2d value;
    ar.read("value", value);
    splitFrustum(value);
    return;
  }

  return GLCamera::execute(ar);
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
    setFov(60.0);
  }
  endTransaction();

  setDraggingSelection(bounds);

  return true;
}

//////////////////////////////////////////////////////////////////////
Matrix GLLookAtCamera::guessProjection(const Viewport& viewport) const
{
  //automatically compute zNear, zFar
  double zNear = 0.00001, zFar = 100;
  {
    auto dir = this->dir.normalized();
    Point3d far_point((dir[0] >= 0) ? bounds.p2[0] : bounds.p1[0], (dir[1] >= 0) ? bounds.p2[1] : bounds.p1[1], (dir[2] >= 0) ? bounds.p2[2] : bounds.p1[2]);
    Point3d near_point((dir[0] >= 0) ? bounds.p1[0] : bounds.p2[0], (dir[1] >= 0) ? bounds.p1[1] : bounds.p2[1], (dir[2] >= 0) ? bounds.p1[2] : bounds.p2[2]);
    Plane camera_plane(dir, this->pos);
    auto nDist = camera_plane.getDistance(near_point);
    auto fDist = camera_plane.getDistance(far_point);

    zNear = nDist <= 0 ? 0.1 : nDist;
    zFar = 2 * fDist - nDist;
  }

  //perspective?
  if (split_frustum == Rectangle2d(0, 0, 1, 1))
  {
    auto aspect_ratio = viewport.getAspectRatio();
    return Matrix::perspective(fov, aspect_ratio, zNear, zFar);
  }
  //if there is a split of the frustum I must use ortho projection
  else
  {
    auto aspect_ratio = viewport.getAspectRatio();
    GLOrthoParams params;
    params.top = +zNear * tan(Utils::degreeToRadiant(fov * 0.5));
    params.bottom = -zNear * tan(Utils::degreeToRadiant(fov * 0.5));
    params.right = +params.top * aspect_ratio;
    params.left = -params.top * aspect_ratio;
    params.zNear = zNear;
    params.zFar = zFar;
    params = params.split(split_frustum);
    return Matrix::frustum(params.left, params.right, params.bottom, params.top, params.zNear, params.zFar);
  }
}

//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getFinalFrustum(const Viewport& viewport) const
{
  Frustum ret;
  ret.setViewport(viewport);
  ret.loadModelview(Matrix::lookAt(this->pos, this->pos + this->dir, this->vup));
  ret.loadProjection(guessProjection(viewport));
  return ret;
}



//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getCurrentFrustum(const Viewport& viewport) const {
  return getFinalFrustum(viewport);
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::move(Point3d vt)
{
  auto dir = this->dir.normalized();
  Point3d far_point ((dir[0] >= 0) ? bounds.p2[0] : bounds.p1[0], (dir[1] >= 0) ? bounds.p2[1] : bounds.p1[1], (dir[2] >= 0) ? bounds.p2[2] : bounds.p1[2]);
  Point3d near_point((dir[0] >= 0) ? bounds.p1[0] : bounds.p2[0], (dir[1] >= 0) ? bounds.p1[1] : bounds.p2[1], (dir[2] >= 0) ? bounds.p1[2] : bounds.p2[2]);
  Plane camera_plane(dir, this->pos);
  auto nDist = camera_plane.getDistance(near_point);
  auto fDist  = camera_plane.getDistance(far_point);

  // when camera inside volume
  // roughly means 64 scroll steps to go through the volume
  auto factor = 1.0;
  if (nDist <= 0)
    factor=(fDist - nDist) / 64;

  // if the camera is far away from the volume, it moves toward the volume faster
  else
    factor= fDist / 16;

  setPosition(this->pos + factor * vt);
}




//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setLookAt(Point3d pos, Point3d dir, Point3d vup)
{
  if (pos == this->pos && dir == this->dir && vup == this->vup)
    return;

  beginUpdate(
    StringTree("SetLookAt", "pos",       pos, "dir",       dir, "vup",       vup),
    StringTree("SetLookAt", "pos", this->pos, "dir", this->dir, "vup", this->vup));
  {
    setPosition(pos);
    setDirection(dir);
    setViewUp(vup);
  }
  endUpdate();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::rotate(double angle_degree, Point3d p0, Point3d p1)
{
  auto axis = (p1 - p0).normalized();

  if (!angle_degree || !axis.module() || !axis.valid())
    return;

  auto T =
    Matrix::lookAt(this->pos, this->pos + this->dir, this->vup) *
    Matrix::rotateAroundCenter(p0, Quaternion(axis, Utils::degreeToRadiant(angle_degree)));

  Point3d pos, dir, vup;
  T.getLookAt(pos, dir, vup);

  beginUpdate(
    StringTree("Rotate", "angle", angle_degree, "p0", p0, "p1", p1),
    StringTree("SetLookAt", "pos", this->pos, "dir", this->dir, "vup", this->vup));
  {
    setLookAt(pos, dir, vup);
  }
  endUpdate();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setDraggingSelection(Position value) {
  dragging.selection = value;
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMousePressEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();
  dragging.frustum = getCurrentFrustum(viewport);
  //beginTransaction();
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
  
  Point2i screen1 = this->mouse.getButton(button).pos; 
  this->mouse.glMouseMoveEvent(evt);
  Point2i screen2 = this->mouse.getButton(button).pos;

  int W= viewport.width;
  int H= viewport.height;

  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton) 
  {

    screen1 = this->mouse.getButton(button).down;

    auto map = FrustumMap(dragging.frustum);

    //https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball
    //https://braintrekking.wordpress.com/2012/08/21/tutorial-of-arcball-without-quaternions/

    auto get_arcball_vector = [&](double x, double y)
    {
      auto P = Point3d(
        2.0 * x / W - 1.0, //from [0,W] -> [-1,+1]
        2.0 * y / H - 1.0, //from [0,H] -> [-1,+1]
        0);

      double d = P.x * P.x + P.y * P.y;

      if (d <= 1 * 1)
        P.z = sqrt(1 * 1 - d);  // Pythagoras
      else
        P = P.normalized();  // nearest point on plane Z=0

      return P;
    };

    auto O = Point3d();
    auto A = get_arcball_vector(screen1[0], screen1[1]);
    auto B = get_arcball_vector(screen2[0], screen2[1]);

    O = (map.modelview.Ti * (map.projection.Ti * PointNd(O, /*point*/1.0))).dropHomogeneousCoordinate().toPoint3();
    A = (map.modelview.Ti * (map.projection.Ti * PointNd(A, /*point*/1.0))).dropHomogeneousCoordinate().toPoint3();
    B = (map.modelview.Ti * (map.projection.Ti * PointNd(B, /*point*/1.0))).dropHomogeneousCoordinate().toPoint3();

    auto va = (A - O).normalized();
    auto vb = (B - O).normalized();

    auto axis = va.cross(vb).normalized();
    auto angle = -acos(va.dot(vb));

    if (angle && axis.module() && axis.valid())
    {
      auto center = dragging.selection.getCentroid().toPoint3();

      auto modelview =
        dragging.frustum.getModelview() *
        Matrix::rotateAroundCenter(center, Quaternion(axis, angle));
       
      Point3d pos, dir, vup;
      modelview.getLookAt(pos, dir, vup);
      setLookAt(pos, dir, vup);
    }

    evt->accept();
    return;
  }

  // pan
  if (this->mouse.getButton(Qt::RightButton).isDown && button==Qt::RightButton) 
  {
    auto dir = this->dir.normalized();
    auto right = dir.cross(vup).normalized();
    auto up = right.cross(dir);

    const double pan_factor = 30;
    auto dx = (((screen1.x - screen2.x) / viewport.width ) * pan_factor) * right;
    auto dy = (((screen1.y - screen2.y) / viewport.height) * pan_factor) * up;
    move(dx + dy);
    evt->accept();
    return;
  }
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMouseReleaseEvent(evt);
  //endTransaction();
  evt->accept();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glWheelEvent(QWheelEvent* evt, const Viewport& viewport)
{
  move((evt->delta() < 0 ? -1 : +1) * this->dir);
  evt->accept();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glKeyPressEvent(QKeyEvent* evt, const Viewport& viewport)
{
  int key=evt->key();

  switch(key)
  {
    case Qt::Key_P:
    {
      StringTree ar(this->getTypeName());
      this->write(ar);
      PrintInfo(ar.toString());
      evt->accept();
      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::write(Archive& ar) const
{
  GLCamera::write(ar);

  ar.write("pos", pos);
  ar.write("dir", dir);
  ar.write("vup", vup);
  ar.write("fov", fov);
  ar.write("split_frustum", split_frustum);

  String s_bounds = this->bounds.toString(/*bInterleave*/false);
  ar.write("bounds", s_bounds);
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::read(Archive& ar)
{
  GLCamera::read(ar);

  ar.read("pos", this->pos);
  ar.read("dir", this->dir);
  ar.read("vup", this->vup);
  ar.read("fov", this->fov);
  ar.read("split_frustum", split_frustum);

  String s_bounds;
  ar.read("bounds", s_bounds);
  this->bounds = BoxNd::fromString(s_bounds,/*bInterleave*/false);
  this->bounds.setPointDim(3);
}

} //namespace Visus
