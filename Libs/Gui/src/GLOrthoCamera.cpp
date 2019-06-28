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
void GLOrthoCamera::mirror(int ref)
{
  if (ref == 0) {
    auto ortho_params=getOrthoParams();
    std::swap(ortho_params.left,ortho_params.right);
    setOrthoParams(ortho_params);
    return;
  }

  if (ref == 1) 
  {
    auto ortho_params=getOrthoParams();
    std::swap(ortho_params.top,ortho_params.bottom);
    setOrthoParams(ortho_params);
    return;
  }
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

  if (this->mouse.getButton(Qt::LeftButton).isDown && this->mouse.getButton(Qt::MidButton).isDown && (button==Qt::LeftButton || button==Qt::MidButton))
  {
    //t1 and t2 are the old position, T1 and T2 are the touch position.
    FrustumMap map=needUnproject();
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

    beginUpdate();
    {
      scale(vs, center);
      translate(-Point2d(tx, ty));
      rotate(-atan2(b, a));
    }
    endUpdate();

    evt->accept();
    return;
  }

  else if (this->mouse.getButton(Qt::LeftButton).isDown && button==Qt::LeftButton)
  {
    FrustumMap map=needUnproject();
    Point2i p1 = last_mouse_pos[button];
    Point2i p2 = this->mouse.getButton(button).pos;
    last_mouse_pos[button] = p2;
    Point2d t1 = map.unprojectPoint(p1.castTo<Point2d>()).toPoint2();
    Point2d T1 = map.unprojectPoint(p2.castTo<Point2d>()).toPoint2();

    beginUpdate();
    {
      translate(t1 - T1);
    }
    endUpdate();

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
  FrustumMap map=needUnproject();
  Point2d center = map.unprojectPoint(Point2d(evt->x(),evt->y())).toPoint2();
  double  vs     = evt->delta()>0 ? (1.0 / default_scale) : (default_scale);

  scale(vs,center);

  evt->accept();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::glKeyPressEvent(QKeyEvent* evt)
{
  int key=evt->key();

  if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) 
  {
    auto ortho_params=getOrthoParams();
    double dx=0,dy=0;
    if (key==Qt::Key_Left ) dx=-ortho_params.getWidth();
    if (key==Qt::Key_Right) dx=+ortho_params.getWidth();
    if (key==Qt::Key_Up   ) dy=+ortho_params.getHeight();
    if (key==Qt::Key_Down ) dy=-ortho_params.getHeight();
    translate(Point2d(dx,dy)); 
    evt->accept(); 
    return; 
  }

  if (key==Qt::Key_Minus || key==Qt::Key_Plus)
  {
    double vs=key=='+'? 1.0/default_scale : default_scale;
    auto ortho_params=getOrthoParams();
    auto center=ortho_params.getCenter().toPoint2();
    scale(vs,center);
    evt->accept();
    return;
  }

  if (key==Qt::Key_M) 
  {
    beginUpdate();
    smooth=smooth? 0 : 0.90;
    endUpdate();
    evt->accept();
    return;
  }
}


////////////////////////////////////////////////////////////////
bool GLOrthoCamera::guessPosition(Position position,int ref)  
{
  Point3d C,X,Y,Z;
  auto bound = BoxNd(3);

  if (position.T.isIdentity())
  {
    C=Point3d(0,0,0);
    X=Point3d(1,0,0);
    Y=Point3d(0,1,0);
    Z=Point3d(0,0,1);
    bound=position.box.toBox3();
  }
  else
  {
    LocalCoordinateSystem lcs(position);
    C=lcs.getCenter();
    X=lcs.getAxis(0);
    Y=lcs.getAxis(1);
    Z=lcs.getAxis(2);

    bound.p2[0]=X.module(); X=X.normalized();
    bound.p2[1]=Y.module(); Y=Y.normalized();
    bound.p2[2]=Z.module(); Z=Z.normalized();
    bound.p1=-1*bound.p2;

    if (X[X.abs().biggest()]<0) X*=-1;
    if (Y[Y.abs().biggest()]<0) Y*=-1;
    if (Z[Z.abs().biggest()]<0) Z*=-1;
  }

  int W = getViewport().width ; if (!W) W=800;
  int H = getViewport().height; if (!H) H=800;

  if (ref==0 || (ref<0 && bound.p1[0]==bound.p2[0] && bound.p1[1]!=bound.p2[1] && bound.p1[2]!=bound.p2[2]))
  {
    beginUpdate();
    this->pos=+C;
    this->dir=-X;
    this->vup=+Z;
    this->rotation_angle = 0.0;
    double Xnear =-bound.p1[0],Xfar  =-bound.p2[0]; if (Xnear==Xfar) {Xnear+=1;Xfar-=1;}
    GLOrthoParams ortho_params = GLOrthoParams(bound.p1[1],bound.p2[1],bound.p1[2],bound.p2[2],Xnear,Xfar);
    ortho_params.fixAspectRatio((double)W/(double)H);
    setOrthoParams(ortho_params);
    endUpdate();
  }
  else if (ref==1 || (ref<0 && bound.p1[0]!=bound.p2[0] && bound.p1[1]==bound.p2[1] && bound.p1[2]!=bound.p2[2]))
  {
    beginUpdate();
    this->pos=+C;
    this->dir=+Y;
    this->vup=+Z;
    this->rotation_angle = 0.0;
    double Ynear =+bound.p1[1],Yfar  =+bound.p2[1]; if (Ynear==Yfar) {Ynear-=1;Yfar+=1;}
    GLOrthoParams ortho_params(bound.p1[0],bound.p2[0],bound.p1[2],bound.p2[2],Ynear,Yfar);
    ortho_params.fixAspectRatio((double)W/(double)H);
    setOrthoParams(ortho_params);
    endUpdate();
  }
  else
  {
    VisusAssert(ref<0 || ref==2);
    beginUpdate();
    this->pos=+C;
    this->dir=-Z;
    this->vup=+Y;
    this->rotation_angle = 0.0;
    double Znear =-bound.p1[2],Zfar  =-bound.p2[2]; if (Znear==Zfar) {Znear+=1;Zfar-=1;}
    GLOrthoParams ortho_params(bound.p1[0],bound.p2[0],bound.p1[1],bound.p2[1],Znear,Zfar);
    ortho_params.fixAspectRatio((double)W/(double)H);
    setOrthoParams(ortho_params);
    endUpdate();
  }

  return true;
}


//////////////////////////////////////////////////
void GLOrthoCamera::setOrthoParams(GLOrthoParams value)
{
  VisusAssert(VisusHasMessageLock());

  timer.stop();

  if (getOrthoParams()==value) 
    return;

  beginUpdate();
  this->ortho_params       = value;
  this->ortho_params_final = value; 
  endUpdate();
}

//////////////////////////////////////////////////
void GLOrthoCamera::setViewport(Viewport new_viewport)
{
  auto old_viewport=this->viewport;
  if (old_viewport==new_viewport) 
    return;

  auto ortho_params=getOrthoParams();
  ortho_params.fixAspectRatio(old_viewport, new_viewport);

  beginUpdate();
  this->viewport=new_viewport;
  setOrthoParams(ortho_params);
  endUpdate();
}

////////////////////////////////////////////////////////////////
Frustum GLOrthoCamera::getFrustum(GLOrthoParams ortho_params) const
{
  Frustum ret;

  ret.loadProjection(ortho_params.getProjectionMatrix(true));
  ret.multProjection(Matrix::rotateAroundCenter(ortho_params.getCenter(),Point3d(0,0,1),rotation_angle));

  ret.loadModelview(Matrix::lookAt(pos,pos+dir,vup));
  
  ret.setViewport(this->viewport);

  return ret;
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::refineToFinal() 
{
  VisusAssert(VisusHasMessageLock());

  if (smooth>0 && !timer.isActive()) 
    timer.start();

  GLOrthoParams ortho_params;
  ortho_params.left    = (smooth) * this->ortho_params.left   + (1-smooth) * this->ortho_params_final.left;
  ortho_params.right   = (smooth) * this->ortho_params.right  + (1-smooth) * this->ortho_params_final.right;
  ortho_params.bottom  = (smooth) * this->ortho_params.bottom + (1-smooth) * this->ortho_params_final.bottom;
  ortho_params.top     = (smooth) * this->ortho_params.top    + (1-smooth) * this->ortho_params_final.top;
  ortho_params.zNear   = (smooth) * this->ortho_params.zNear  + (1-smooth) * this->ortho_params_final.zNear;
  ortho_params.zFar    = (smooth) * this->ortho_params.zFar   + (1-smooth) * this->ortho_params_final.zFar;

  if (ortho_params==this->ortho_params || ortho_params==this->ortho_params_final)  
  {
    ortho_params=this->ortho_params_final;
    timer.stop();
  }

  //limit the zoom to actual pixels visible 
  if ((max_zoom>0 || min_zoom>0) && getViewport().valid())
  {
    Point2d pixel_per_sample(
      (double)getViewport().width /ortho_params.getWidth (),
      (double)getViewport().height/ortho_params.getHeight());

    if (max_zoom>0 && std::max(pixel_per_sample[0],pixel_per_sample[1]) > max_zoom)
    {
      timer.stop();
      return;
    }

    if (min_zoom>0 && std::min(pixel_per_sample[0],pixel_per_sample[1]) < min_zoom)
    {
      timer.stop();
      return;
    }
  }

  beginUpdate();
  this->ortho_params=ortho_params;
  endUpdate();
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::translate(Point2d vt)
{
  if (!vt[0] && !vt[1])
    return;

  this->ortho_params_final.translate(Point3d(vt));
  refineToFinal();
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::rotate(double quantity)
{
  if (!quantity)
    return;

  beginUpdate();
  this->rotation_angle += bDisableRotation ? 0.0 : quantity;
  endUpdate();
}


////////////////////////////////////////////////////////////////
void GLOrthoCamera::scale(double vs,Point2d center)
{
  if (vs==1 || vs==0)
    return;

  this->ortho_params_final.scaleAroundCenter(Point3d(vs,vs,1.0),Point3d(center));
  refineToFinal();
}


 ////////////////////////////////////////////////////////////////
void GLOrthoCamera::writeToObjectStream(ObjectStream& ostream) 
{
  if (ostream.isSceneMode())
  {
    ostream.writeInline("type", "ortho");
    // TODO generalize keyframes interpolation
    ostream.pushContext("keyframes");
    ostream.writeInline("interpolation", "linear");

    // TODO Loop through the keyframes 
    ostream.pushContext("keyframe");
    ostream.writeInline("time", "0");

    // write camera data
    ostream.write("pos", pos.toString());
    ostream.write("dir", dir.toString());
    ostream.write("vup", vup.toString());

    ostream.pushContext("ortho_params");
    ortho_params.writeToObjectStream(ostream);
    ostream.popContext("ortho_params");

    ostream.popContext("keyframe");
    // end keyframes loop

    ostream.popContext("keyframes");
    return;
  }

  GLCamera::writeToObjectStream(ostream);

  ostream.write("default_scale",cstring(default_scale));
  ostream.write("disable_rotation",cstring(bDisableRotation));
  ostream.write("rotation_angle", cstring(rotation_angle));
  ostream.write("max_zoom",cstring(max_zoom));
  ostream.write("min_zoom",cstring(min_zoom));
  ostream.write("smooth",cstring(smooth));

  ostream.write("pos",pos.toString());
  ostream.write("dir",dir.toString());
  ostream.write("vup",vup.toString());

  ostream.pushContext("ortho_params");
  ortho_params.writeToObjectStream(ostream);
  ostream.popContext("ortho_params");
}

////////////////////////////////////////////////////////////////
void GLOrthoCamera::readFromObjectStream(ObjectStream& istream) 
{
  if (istream.isSceneMode())
  {
    pos = Point3d(istream.read("pos", "0  0  0"));
    dir = Point3d(istream.read("dir", "0  0 -1"));
    vup = Point3d(istream.read("vup", "0  1  0"));

    rotation_angle = cdouble(istream.read("rotation_angle"));

    istream.pushContext("ortho_params");
    {
      GLOrthoParams value;
      value.readFromObjectStream(istream);
      this->ortho_params = value;
      this->ortho_params_final = value;
    }
    istream.popContext("ortho_params");
    return;
  }

  GLCamera::readFromObjectStream(istream);

  pos=Point3d(istream.read("pos","0  0  0"));
  dir=Point3d(istream.read("dir","0  0 -1"));
  vup=Point3d(istream.read("vup","0  1  0"));

  default_scale=cdouble(istream.read("default_scale"));
  bDisableRotation=cbool(istream.read("disable_rotation"));
  rotation_angle=cdouble(istream.read("rotation_angle"));
  max_zoom=cdouble(istream.read("max_zoom"));
  min_zoom=cdouble(istream.read("min_zoom"));
  smooth=cdouble(istream.read("smooth"));

  istream.pushContext("ortho_params");
  {
    GLOrthoParams value;
    value.readFromObjectStream(istream);
    this->ortho_params       = value;
    this->ortho_params_final = value;
  }
  istream.popContext("ortho_params");
}
  


} //namespace

