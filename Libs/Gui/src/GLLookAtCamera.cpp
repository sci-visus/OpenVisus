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
void GLLookAtCamera::setViewport(Viewport new_viewport)
{
  auto old_viewport=getViewport();

  if (old_viewport==new_viewport)
    return;

  beginUpdate();
  ortho_params.fixAspectRatio(old_viewport, new_viewport);
  this->viewport=new_viewport;

  if (bAutoOrthoParams)
    guessOrthoParams();

  endUpdate();
}



//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setCenterOfRotation(Point3d value)
{
  if (this->centerOfRotation==value)
    return;

  beginUpdate();
  this->centerOfRotation=value;
  this->dir=(this->centerOfRotation-this->pos).normalized();

  if (bAutoOrthoParams)
    guessOrthoParams();

  endUpdate();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setUseOrthoProjection(bool value)
{
  if (this->bUseOrthoProjection==value)
    return;
 
  beginUpdate();
  this->bUseOrthoProjection=value;

  if (bAutoOrthoParams)
    guessOrthoParams();

  endUpdate();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setOrthoParams(GLOrthoParams value) 
{
  if (getOrthoParams()==value)
    return;

  setProperty(this->ortho_params, value);
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setAutoOrthoParams(bool value) 
{
  if (value==this->bAutoOrthoParams)
    return;
 
  beginUpdate();
  this->bAutoOrthoParams=value;

  if (bAutoOrthoParams)
    guessOrthoParams();

  endUpdate();
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

  if (this->mouse.getButton(Qt::LeftButton).isDown && this->mouse.getButton(Qt::MidButton).isDown && (button==Qt::LeftButton || button==Qt::MidButton)) // 2-finger zoom/pan/rotate
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

    beginUpdate();
    {
      rotate(center,angle);
      pan(center,center+Point2d(tx,ty));
      forward((a < 1 ? -1 : +1) * guessForwardFactor());
    }
    endUpdate();
    evt->accept();
    return;
  }
  else if (this->mouse.getButton(Qt::LeftButton).isDown && button==Qt::LeftButton) // rotate
  {
    Point2i screen_p1 = last_mouse_pos[button];
    Point2i screen_p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = screen_p2;

    double R = ortho_params.getWidth() / 2.0;

    // project the center of rotation to the screen
    Frustum temp;
    temp.setViewport(getViewport());
    temp.loadProjection(Matrix::perspective(60, W/(double)H, ortho_params.zNear, ortho_params.zFar));
    temp.loadModelview(Matrix::lookAt(this->pos, this->pos+this->dir, this->vup));
    FrustumMap map(temp);
    Point3d center = map.unprojectPointToEye(map.projectPoint(centerOfRotation));
    Point3d p1 = map.unprojectPointToEye(screen_p1.castTo<Point2d>());
    Point3d p2 = map.unprojectPointToEye(screen_p2.castTo<Point2d>());
    double x1 = p1[0] - center[0], y1 = p1[1] - center[1];
    double x2 = p2[0] - center[0], y2 = p2[1] - center[1];
    double l1 = x1*x1 + y1*y1, l2 = x2*x2 + y2*y2;
    if (l1 < R*R/2)
        p1[2] = sqrt(R*R - l1);
    else
        p1[2] = R*R / 2 / sqrt(l1);

    if (l2 < R*R/2)
        p2[2] = sqrt(R*R - l2);
    else
        p2[2] = R*R / 2 / sqrt(l2);

    auto M = Matrix::lookAt(this->pos, this->pos + this->dir, this->vup).invert();
    Point3d a = (M * p1 - M * center).normalized();
    Point3d b = (M * p2 - M * center).normalized();
    Point3d axis = a.cross(b).normalized();
    double angle = acos(a.dot(b));

    if (axis.module() && axis.valid())
    {
      beginUpdate();

      this->quaternion= Quaternion(axis,angle*defaultRotFactor) * this->quaternion;

      if (bAutoOrthoParams)
        guessOrthoParams();

      endUpdate();
    }

    evt->accept();
    return;
  }

  // pan
  if (this->mouse.getButton(Qt::RightButton).isDown && button==Qt::RightButton) 
  {
    Point2i p1 = last_mouse_pos[button];
    Point2i p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = p2;

    pan(p1.castTo<Point2d>(),p2.castTo<Point2d>());

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
  forward((evt->delta() < 0 ? -1 : +1) * guessForwardFactor());

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
      auto ray=FrustumMap(getFrustum()).getRay(Point2d(W,H));

      RayBoxIntersection intersection(ray,bound);

      if (intersection.valid)
        this->setCenterOfRotation(ray.getPoint(0.5*(std::max(intersection.tmin,0.0)+intersection.tmax)).toPoint3());

      evt->accept();
      return ;
    }
    case Qt::Key_P:
    {
      StringTree out(this->getTypeName());
      this->writeTo(out);
      VisusInfo() << out.toXmlString();
      evt->accept();
      return;
    }

    case Qt::Key_M:
    {
      setUseOrthoProjection(!this->bUseOrthoProjection);
      evt->accept();
      return;
    }
  }
}


//////////////////////////////////////////////////////////////////////
bool GLLookAtCamera::guessPosition(Position position,int ref) 
{
  auto bound=position.toAxisAlignedBox();
  bound.setPointDim(3);

  beginUpdate();
  {
    this->bound=bound;

    this->bUseOrthoProjection=false;
    this->centerOfRotation=bound.center().toPoint3();

    if (ref<0)
    {
      this->pos = centerOfRotation +2.1*bound.size().toPoint3();
      this->dir =(centerOfRotation-pos).normalized();
      this->vup = Point3d(0,0,1);
    }
    else
    {
      const std::vector<Point3d> Axis = {Point3d(1,0,0),Point3d(0,1,0),Point3d(0,0,1)};
      this->pos=centerOfRotation + 2.1f * bound.maxsize()*Axis[ref];
      this->dir=-Axis[ref];
      this->vup = std::vector<Point3d>({Point3d(0,0,1),Point3d(0,0,1),Point3d(0,1,0)})[ref];
    }

    //at the beginning no rotation
    this->quaternion= Quaternion();
  }

  guessOrthoParams();

  endUpdate();

  return true;
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setBounds(BoxNd value)
{
  beginUpdate();
  this->bound = value;
  this->centerOfRotation = value.center().toPoint3();
  endUpdate();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::setLookAt(Point3d pos, Point3d center, Point3d vup)
{
  beginUpdate();
  {
    this->pos=pos;
    this->dir= (center - pos).normalized();
    this->vup= vup;
    this->centerOfRotation = center;
    this->quaternion= Quaternion();
  }
  guessOrthoParams();
  endUpdate();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::forward(double factor)
{
  beginUpdate();
  this->pos += this->dir * factor;
  if (bAutoOrthoParams)
    guessOrthoParams();
  endUpdate();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::pan(Point2d screen_p1,Point2d screen_p2)
{
  beginUpdate();

  Point3d right = dir.cross(vup).normalized();
  Point3d up    = right.cross(dir);

  // the pan speed depends on forwardFactor to ensure a consistent perceptual pan speed regardless of
  // how far the camera is from the volume

  if (right.valid())
    this->pos -= (screen_p2[0] - screen_p1[0]) / getViewport().width * right * guessForwardFactor() * defaultPanFactor;

  if (up.valid())
    this->pos -= (screen_p2[1] - screen_p1[1]) / getViewport().height * up * guessForwardFactor() * defaultPanFactor;

  if (bAutoOrthoParams)
    guessOrthoParams();

  endUpdate();
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::rotate(Point2d screen_center,double angle)
{
  if (this->bDisableRotation || !angle) 
    return ;

  beginUpdate();

  int W=getViewport().width;
  int H=getViewport().height;

  Frustum temp;
  temp.setViewport(getViewport());
  temp.loadProjection(getFrustum().getProjection());
  temp.loadModelview(Matrix::lookAt(this->pos,this->pos+this->dir,this->vup));

  FrustumMap map(temp);
  Point3d world_center=map.unprojectPoint(screen_center);
  auto Trot = Matrix::translate(+1*world_center) * Matrix::rotateAroundAxis(dir,angle) * Matrix::translate(-1*world_center);
  auto mv= Matrix::lookAt(this->pos,this->pos+this->dir,this->vup) * Trot;
  Point3d POS,DIR,VUP;mv.getLookAt(POS,DIR,VUP);
  if (!POS.valid() || !DIR.valid() || !VUP.valid() || (POS==pos && DIR==dir && VUP==vup)) return;

  this->pos=POS;
  this->dir=DIR;
  this->vup=VUP;

  if (bAutoOrthoParams)
    guessOrthoParams();

  endUpdate();
}

//////////////////////////////////////////////////////////////////////
Frustum GLLookAtCamera::getFrustum() const
{
  Frustum ret;

  ret.setViewport(viewport);

  ret.loadModelview(Matrix::lookAt(this->pos,this->pos+this->dir,this->vup));
  ret.multModelview(Matrix::rotateAroundCenter(centerOfRotation,quaternion.getAxis(),quaternion.getAngle()));

  ret.loadProjection(ortho_params.getProjectionMatrix(bUseOrthoProjection));

  return ret;
}

//////////////////////////////////////////////////////////////////////
std::pair<double,double> GLLookAtCamera::guessNearFarDistance() const {

  Matrix  modelview_inverse = getFrustum().getModelview().invert();
  Point3d camera_pos        =  modelview_inverse.getCol(3).toPoint3();
  Point3d lookat_dir        = -modelview_inverse.getCol(2).toPoint3();

  Plane   camera_plane(lookat_dir, camera_pos);

  // compute the distance from the camera to the bounding box of the volume
  Point3d volume_far_point ((lookat_dir[0] >= 0) ? bound.p2[0] : bound.p1[0], (lookat_dir[1] >= 0) ? bound.p2[1] : bound.p1[1], (lookat_dir[2] >= 0) ? bound.p2[2] : bound.p1[2]);
  Point3d volume_near_point((lookat_dir[0] >= 0) ? bound.p1[0] : bound.p2[0], (lookat_dir[1] >= 0) ? bound.p1[1] : bound.p2[1], (lookat_dir[2] >= 0) ? bound.p1[2] : bound.p2[2]);

  double near_dist = camera_plane.getDistance(volume_near_point);
  double far_dist  = camera_plane.getDistance(volume_far_point );

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
  beginUpdate();

  auto   pair     =guessNearFarDistance();
  double near_dist=pair.first;
  double far_dist =pair.second;

  double ratio = getViewport().width / (double)getViewport().height;
  const double fov=60.0;
  ortho_params.zNear  = near_dist <= 0? 0.1 : near_dist;
  ortho_params.zFar   = far_dist + far_dist - near_dist;
  ortho_params.top    = ortho_params.zNear * tan(fov * Math::Pi / 360.0);
  ortho_params.bottom = -ortho_params.top;
  ortho_params.left   = ortho_params.bottom * ratio;
  ortho_params.right  = ortho_params.top    * ratio;

  endUpdate();
}


//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::writeTo(StringTree& out) 
{
  GLCamera::writeTo(out);

  out.writeValue("bound",bound.toString(/*bInterleave*/false));
  out.writeValue("pos",pos.toString());
  out.writeValue("dir",dir.toString());
  out.writeValue("vup",vup.toString());
  out.writeValue("quaternion",quaternion.toString());
  out.writeValue("centerOfRotation",centerOfRotation.toString());
  out.writeValue("defaultRotFactor",cstring(defaultRotFactor));
  out.writeValue("defaultPanFactor",cstring(defaultPanFactor));
  out.writeValue("disableRotation",cstring(bDisableRotation));
  out.writeValue("bUseOrthoProjection",cstring(bUseOrthoProjection));
  out.writeValue("bAutoOrthoParams",cstring(bAutoOrthoParams));

  out.writeObject("ortho_params", ortho_params);
}

//////////////////////////////////////////////////////////////////////
void GLLookAtCamera::readFrom(StringTree& in) 
{
  GLCamera::readFrom(in);

  bound = BoxNd::fromString(in.readValue("bound"),/*bInterleave*/false);
  bound.setPointDim(3);
  pos              = Point3d::fromString(in.readValue("pos"));
  dir              = Point3d::fromString(in.readValue("dir"));
  vup              = Point3d::fromString(in.readValue("vup"));
  centerOfRotation = Point3d::fromString(in.readValue("centerOfRotation"));
  quaternion       = Quaternion::fromString(in.readValue("quaternion"));
  defaultRotFactor =cdouble(in.readValue("defaultRotFactor"));
  defaultPanFactor =cdouble(in.readValue("defaultPanFactor"));
  bDisableRotation =cbool  (in.readValue("disableRotation"));
  bUseOrthoProjection=cbool (in.readValue("bUseOrthoProjection"));
  bAutoOrthoParams=cbool(in.readValue("bAutoOrthoParams","1"));

  in.readObject("ortho_params", ortho_params);
}
  



} //namespace Visus
