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
GLLookAtCamera::GLLookAtCamera() {
}

//////////////////////////////////////////////////
GLLookAtCamera::~GLLookAtCamera() {
}

//////////////////////////////////////////////////
void GLLookAtCamera::execute(Archive& ar)
{
  if (ar.name == "SetPosition") {
    Point3d value;
    ar.read("value", value);
    setPosition(value);
    return;
  }

  if (ar.name == "SetCenter") {
    Point3d value;
    ar.read("value", value);
    setCenter(value);
    return;
  }

  if (ar.name == "SetViewUp") {
    Point3d value;
    ar.read("value", value);
    setViewUp(value);
    return;
  }

  if (ar.name == "SetLookAt") {
    Point3d pos,center,vup;
    ar.read("pos", pos);
    ar.read("center", center);
    ar.read("vup", vup);
    setLookAt(pos, center,vup);
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

  if (ar.name == "SetZNear") {
    double value;
    ar.read("value", value, 0.0001);
    setZNear(value);
    return;
  }

  if (ar.name == "SetZFar") {
    double value;
    ar.read("value", value, 100);
    setZFar(value);
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

  Point3d pos, center = bounds.center().toPoint3(), vup;

  if (ref < 0)
  {
    pos = center + 2.1 * bounds.size().toPoint3();
    vup = Point3d(0, 0, 1);
  }
  else
  {
    const std::vector<Point3d> Axis = { Point3d(1,0,0),Point3d(0,1,0),Point3d(0,0,1) };
    const std::vector<Point3d> Vup  = { Point3d(0,0,1),Point3d(0,0,1),Point3d(0,1,0) };
    pos = center + 2.1f * bounds.maxsize() * Axis[ref];
    vup = Vup[ref];
  }

#if 1
  auto zNear = bounds.maxsize() *0.01;
  auto zFar  = bounds.maxsize() * 10.00;
#else
  auto dir = (center - pos).normalized();
  auto fPoint = Point3d((dir[0] >= 0) ? bounds.p2[0] : bounds.p1[0], (dir[1] >= 0) ? bounds.p2[1] : bounds.p1[1], (dir[2] >= 0) ? bounds.p2[2] : bounds.p1[2]);
  auto nPoint = Point3d((dir[0] >= 0) ? bounds.p1[0] : bounds.p2[0], (dir[1] >= 0) ? bounds.p1[1] : bounds.p2[1], (dir[2] >= 0) ? bounds.p1[2] : bounds.p2[2]);
  auto camera_plane= Plane(dir, pos);
  auto nDist = camera_plane.getDistance(nPoint);
  auto fDist = camera_plane.getDistance(fPoint);
  auto zNear = nDist <= 0 ? 0.1 : nDist;
  auto zFar = 2 * fDist - nDist;
#endif

  beginTransaction();
  {
    setLookAt(pos,center,vup);
    setFov(60.0);
    setZNear(zNear);
    setZFar(zFar);
  }
  endTransaction();

  //this is for browsing
  setCameraSelection(bounds);

  return true;
}

//////////////////////////////////////////////////////////////////////
Matrix GLLookAtCamera::getProjection(const Viewport& viewport) const
{
  auto aspect_ratio = viewport.getAspectRatio();

  //perspective?
  if (split_frustum == Rectangle2d(0, 0, 1, 1))
  {
    return Matrix::perspective(fov, aspect_ratio, zNear, zFar);
  }
  //if there is a split of the frustum I must use ortho projection
  else
  {
    GLOrthoParams params;
    params.top    = +zNear * tan(Utils::degreeToRadiant(fov * 0.5));
    params.bottom = -zNear * tan(Utils::degreeToRadiant(fov * 0.5));
    params.right  = +params.top * aspect_ratio;
    params.left   = -params.top * aspect_ratio;
    params.zNear  = zNear;
    params.zFar   = zFar;
    params = params.split(split_frustum);
    return Matrix::frustum(params.left, params.right, params.bottom, params.top, params.zNear, params.zFar);
  }
}

//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getFinalFrustum(const Viewport& viewport) const
{
  Frustum ret;
  ret.setViewport(viewport);
  ret.loadProjection(getProjection(viewport));
  ret.loadModelview(Matrix::lookAt(this->pos, this->center, this->vup));
  return ret;
}

//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getCurrentFrustum(const Viewport& viewport) const {
  return getFinalFrustum(viewport);
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setLookAt(Point3d pos, Point3d center, Point3d vup)
{
  if (pos == this->pos && center == this->center && vup == this->vup)
    return;

  beginUpdate(
    StringTree("SetLookAt", "pos",       pos, "center",       center, "vup",       vup),
    StringTree("SetLookAt", "pos", this->pos, "center", this->center, "vup", this->vup));
  {
    setPosition(pos);
    setCenter(center);
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

  auto old_pos = this->pos, old_center = this->center, old_vup = this->vup;

  Point3d new_pos, new_center, new_vup;
  {
    auto T =
      Matrix::lookAt(old_pos, old_center, old_vup) *
      Matrix::rotateAroundCenter(p0, Quaternion(axis, Utils::degreeToRadiant(angle_degree)));

    auto lookDistance = (old_center - old_pos).module();
    T.getLookAt(new_pos, new_center, new_vup, lookDistance);
  }

  beginUpdate(
    StringTree("Rotate", "angle", angle_degree, "p0", p0, "p1", p1),
    StringTree("SetLookAt", "pos", old_pos, "center", old_center, "vup", old_vup));
  {
    setLookAt(new_pos, new_center, new_vup);
  }
  endUpdate();
}


/////////////////////////////////////////////////////////////////////////////////////
// see https://github.com/Twinklebear/arcball-cpp
class GLLookAtCamera::ArcballCamera
{
public:

  double     lookDistance = 1.0;
  Quaternion rotation;
  Point3d    center_of_rotation;

  //constructor
  ArcballCamera(const Point3d& Eye , const Point3d& Center, const Point3d& Up)
  {
    auto forward = (Center - Eye).normalized();
    auto side = forward.cross(Up.normalized()).normalized();
    auto up = side.cross(forward).normalized();
    side = forward.cross(up).normalized();

    this->lookDistance = (Center - Eye).module();
    this->rotation = Matrix(side, up, -forward).transpose().toQuaternion();
    this->center_of_rotation = Center;
  }

  //getModelview
  Matrix getModelview() const {
    return Matrix::translate(Point3d(0, 0, -this->lookDistance)) *
      Matrix::rotate(this->rotation) *
      Matrix::translate(-this->center_of_rotation);
  }

  //getLookAt
  void getLookAt(Point3d& pos,Point3d& center, Point3d& vup)
  {
    getModelview().getLookAt(pos, center, vup, this->lookDistance);
  }

  //rotate
  void rotate(Point2d s1, Point2d s2)
  {
    auto toArcBall = [](const Point2d& p)
    {
      double dist = p.dot(p);
      if (dist <= 1.f) 
        return Quaternion(0.0, p.x, p.y, std::sqrt(1.f - dist));
      else {
        auto proj = p.normalized();
        return Quaternion(0.0, proj.x, proj.y, 0.f);
      }
    };

    this->rotation = toArcBall(s2) * toArcBall(s1) * this->rotation;
  }

};

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setCameraSelection(Position value) 
{
  this->selection = value;
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMousePressEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();

  //TODO: how to use this?
  //auto center_of_rotation = this->selection.getCentroid().toPoint3();

  arcball.reset(new ArcballCamera(this->pos, this->center, this->vup));

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
  
  Point2d screen1 = this->mouse.getButton(button).pos.castTo<Point2d>();
  this->mouse.glMouseMoveEvent(evt);
  Point2d screen2 = this->mouse.getButton(button).pos.castTo<Point2d>();

  double W= viewport.width;
  double H= viewport.height;

  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton) 
  {
    screen1 = Point2d(2.0 * screen1[0] / W - 1.0, 2.0 * screen1[1] / H - 1.0);
    screen2 = Point2d(2.0 * screen2[0] / W - 1.0, 2.0 * screen2[1] / H - 1.0);
    arcball->rotate(screen1, screen2);

    Point3d pos, center, vup;
    arcball->getLookAt(pos, center, vup);
    setLookAt(pos, center, vup);

    evt->accept();
    return;
  }

  // pan
  if (this->mouse.getButton(Qt::RightButton).isDown && button==Qt::RightButton) 
  {
    auto map = FrustumMap(getCurrentFrustum(viewport));

    auto zCenter = map.toNormalizedScreenCoordinates(this->center)[2];
    auto w1=map.unprojectPoint(screen1, zCenter);
    auto w2=map.unprojectPoint(screen2, zCenter);
    auto vt = w1 - w2;

    beginTransaction();
    {
      setPosition(this->pos + vt);
      setCenter(this->center + vt);
    }
    endTransaction();

    evt->accept();
    return;
  }
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMouseReleaseEvent(evt);
  //endTransaction();
  arcball.reset();
  evt->accept();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glWheelEvent(QWheelEvent* evt, const Viewport& viewport)
{
  auto sign = evt->delta() < 0 ? -1 : +1;
  auto vt = sign * 0.008 * (this->center - this->pos);

  beginTransaction();
  {
    setPosition(this->pos + vt);
    setCenter(this->center+vt);
  }
  endTransaction();
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
  ar.write("center", center);
  ar.write("vup", vup);
  ar.write("fov", fov);
  ar.write("znear", zNear);
  ar.write("zfar", zFar);
  ar.write("split_frustum", split_frustum);
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::read(Archive& ar)
{
  GLCamera::read(ar);

  ar.read("pos", this->pos);
  ar.read("center", this->center);
  ar.read("vup", this->vup);
  ar.read("fov", this->fov);
  ar.read("znear", this->zNear);
  ar.read("zfar", this->zFar);
  ar.read("split_frustum", split_frustum);
}

} //namespace Visus
