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
void GLLookAtCamera::setLookAt(Point3d pos, Point3d center, Point3d vup) {

  //useless call
  if (this->pos == pos && this->center == center && this->vup == vup)
    return;

  beginTransaction();
  {
    setPos(pos);
    setCenter(center);
    setVup(vup);
  }
  endTransaction();
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
Matrix GLLookAtCamera::getModelview() const
{
  return
    Matrix::lookAt(this->pos, this->center, this->vup)
    * Matrix::rotateAroundCenter(this->center_of_rotation, this->rotation);
}

//////////////////////////////////////////////////
void GLLookAtCamera::execute(Archive& ar)
{
  if (ar.name == "SetPosition" || ar.name=="SetPos") {
    Point3d value;
    ar.read("value", value);
    setPos(value);
    return;
  }

  if (ar.name == "SetCenter") {
    Point3d value;
    ar.read("value", value);
    setCenter(value);
    return;
  }

  if (ar.name == "SetViewUp" || ar.name == "SetVup") {
    Point3d value;
    ar.read("value", value);
    setVup(value);
    return;
  }

  if (ar.name == "SetCenterOfRotation") {
    Point3d value;
    ar.read("value", value);
    setCenterOfRotation(value);
    return;
  }

  if (ar.name == "SetRotation") {
    Quaternion value;
    ar.read("value", value);
    setRotation(value);
    return;
  }

  if (ar.name == "SetLookAt") {
    Point3d pos,center,vup;
    ar.read("pos", pos);
    ar.read("center", center);
    ar.read("vup", vup);
    setLookAt(pos, center, vup);
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

  Point3d pos;
  Point3d center = bounds.center().toPoint3();
  Point3d vup;
  Point3d dir;

  if (ref < 0)
  {
    pos = center + 2.1 * bounds.size().toPoint3();
    vup = Point3d(0, 0, 1);
    dir = (center - pos).normalized();
  }
  else
  {
    const std::vector<Point3d> Axis = { Point3d(1,0,0),Point3d(0,1,0),Point3d(0,0,1) };
    const std::vector<Point3d> Vup  = { Point3d(0,0,1),Point3d(0,0,1),Point3d(0,1,0) };
    pos = center + 2.1f * bounds.maxsize() * Axis[ref];
    vup = Vup[ref];
    dir = -Axis[ref];
  }

#if 0
  auto zNear = bounds.maxsize() *  0.01;
  auto zFar  = bounds.maxsize() * 10.00;
#else
  auto camera_plane = Plane(dir, pos);
  auto fPoint = Point3d((dir[0] >= 0) ? bounds.p2[0] : bounds.p1[0], (dir[1] >= 0) ? bounds.p2[1] : bounds.p1[1], (dir[2] >= 0) ? bounds.p2[2] : bounds.p1[2]);
  auto nPoint = Point3d((dir[0] >= 0) ? bounds.p1[0] : bounds.p2[0], (dir[1] >= 0) ? bounds.p1[1] : bounds.p2[1], (dir[2] >= 0) ? bounds.p1[2] : bounds.p2[2]);
  auto nDist = camera_plane.getDistance(nPoint);
  auto fDist = camera_plane.getDistance(fPoint);
  auto zNear = nDist <= 0 ? 0.1 : nDist;
  auto zFar  = 2 * fDist - nDist;
#endif

  auto center_of_rotation = center;
  auto rotation = Quaternion();

  beginTransaction();
  {
    setFov(60.0);
    setZNear(zNear);
    setZFar(zFar);
    setPos(pos);
    setCenter(center);
    setVup(vup);
    setCenterOfRotation(center_of_rotation);
    setRotation(rotation);
  }
  endTransaction();

  return true;
}




//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMousePressEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMousePressEvent(evt);
  evt->accept();

  //I want the actions to be sent continously
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

  //trackback rotation
  if (this->mouse.getButton(Qt::LeftButton).isDown && button == Qt::LeftButton) 
  {
    Matrix  Ti =
      (
        Frustum::getViewportDirectTransformation(viewport)
        * getProjection(viewport)
        * Matrix::lookAt(this->pos, this->center, this->vup)
        * Matrix::translate(+1 * this->center_of_rotation) //note: going 'inside' rotation
       )
      .invert();

    auto a = (Ti * Point3d(screen1)).normalized();
    auto b = (Ti * Point3d(screen2)).normalized();
    auto axis = a.cross(b).normalized();
    double angle = acos(a.dot(b));

    const double defaultRotFactor = 1.0;

    if (axis.module() && axis.valid())
      setRotation(Quaternion(axis, angle * defaultRotFactor) * this->rotation) ;


    evt->accept();
    return;
  }

  // pan
  if (this->mouse.getButton(Qt::RightButton).isDown && button==Qt::RightButton) 
  {
    auto Ti = (
      Frustum::getViewportDirectTransformation(viewport)
      * getProjection(viewport)
      * Matrix::lookAt(this->pos, this->center, this->vup) //NOTE: excluding the rotation part;
      ).invert();

    double defaultPanFactor = 1.0;
    Point3d vt = (Ti * Point3d(screen2) - Ti * Point3d(screen1)) * defaultPanFactor;

    if (vt.valid())
      setLookAt(pos - vt, center - vt, vup);

    evt->accept();
    return;
  }
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glMouseReleaseEvent(QMouseEvent* evt, const Viewport& viewport)
{
  this->mouse.glMouseReleaseEvent(evt);

  //I want the actions to be sent continously
  //endTransaction();
  evt->accept();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glWheelEvent(QWheelEvent* evt, const Viewport& viewport)
{
  const double defaultZoomFactor = 1.1;
  setFov(fov * ((evt->delta() > 0) ? (1.0 / defaultZoomFactor) : (defaultZoomFactor)));
  evt->accept();
}



//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::glKeyPressEvent(QKeyEvent* evt, const Viewport& viewport)
{
  int key=evt->key();

  switch(key)
  {
    //print info
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


/////////////////////////////////////////////
#if 0
void GLLookAtCamera::glDoubleClickEvent(QMouseEvent* evt, const Viewport& viewport)
{
  int button = args->button;
  double x = args->x;
  double y = args->y;

  int W = viewport.width / 2;
  int H = viewport.height / 2;
  Ray r = frustum->getRay(Point(x, y));
  double tmin, tmax;
  r.intersectBox(tmin, tmax, bound);

  if (tmin > 0)
    this->setCenterOfRotation(r.getPoint(tmin + (tmax - tmin) / 2.));
}
#endif


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::write(Archive& ar) const
{
  GLCamera::write(ar);

  ar.write("fov", fov);
  ar.write("znear", zNear);
  ar.write("zfar", zFar);

  ar.write("pos", pos);
  ar.write("center", center);
  ar.write("vup", vup);

  ar.write("center_of_rotation", center_of_rotation);
  ar.write("rotation", rotation);
  
  if (split_frustum != Rectangle2d(0, 0, 1, 1))
    ar.write("split_frustum", split_frustum);
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::read(Archive& ar)
{
  GLCamera::read(ar);

  ar.read("fov", this->fov);
  ar.read("znear", this->zNear);
  ar.read("zfar", this->zFar);

  ar.read("pos", this->pos);
  ar.read("center", this->center);
  ar.read("vup", this->vup);

  ar.read("center_of_rotation", center_of_rotation,this->center);
  ar.read("rotation", rotation, Quaternion());

  ar.read("split_frustum", split_frustum, Rectangle2d(0, 0, 1, 1));
}

} //namespace Visus
