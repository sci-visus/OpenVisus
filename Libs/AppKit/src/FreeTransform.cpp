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

#include <Visus/FreeTransform.h>
#include <Visus/GuiFactory.h>
#include <Visus/GLObjects.h>


#include <QApplication>

namespace Visus {

using namespace GuiFactory;

////////////////////////////////////////////////////////////////////////
static bool NearBy(const Point2d& screen_p1,const Point2d& screen_p2,int tolerance=6) 
{
  return screen_p1.distance(screen_p2)<tolerance;
}


////////////////////////////////////////////////////////////////////////////
bool FreeTransform::setObject(Position obj,bool bEmitSignal)
{
  obj = Position(
    obj.getTransformation().withSpaceDim(4), 
    obj.getBoxNd().withPointDim(3));

  setProperty(this->obj, obj);

  if (!dragging.type)
    this->lcs=LocalCoordinateSystem(this->obj).toUniformSize();

  if (bEmitSignal)
    objectChanged.emitSignal(obj);

  return true;
}


////////////////////////////////////////////////////////////////////////////
void FreeTransform::doTranslate(Point3d vt) 
{
  auto lcs=dragging.type? dragging.lcs : this->lcs;
  auto obj=dragging.type? dragging.begin : this->obj;

  vt=
    vt[0]*lcs.getAxis(0).normalized()+
    vt[1]*lcs.getAxis(1).normalized()+
    vt[2]*lcs.getAxis(2).normalized();

  auto T= Matrix::translate(vt);

  
  setObject(Position(T,obj),true);

  if (dragging.type)
    this->lcs=LocalCoordinateSystem(T,dragging.lcs);

}

////////////////////////////////////////////////////////////////////////////
void FreeTransform::doRotate(Point3d vr) 
{
  auto lcs=dragging.type? dragging.lcs   : this->lcs;
  auto obj=dragging.type? dragging.begin : this->obj;

  auto T= Matrix::translate(+lcs.getCenter()) *
    Matrix::rotateAroundAxis(lcs.getAxis(2).normalized(),vr[2]) *
    Matrix::rotateAroundAxis(lcs.getAxis(1).normalized(),vr[1]) *
    Matrix::rotateAroundAxis(lcs.getAxis(0).normalized(),vr[0]) *
    Matrix::translate(-lcs.getCenter());

  setObject(Position(T,obj),true);

  if (dragging.type)
    this->lcs=LocalCoordinateSystem(T,dragging.lcs);
}

////////////////////////////////////////////////////////////////////////////
void FreeTransform::doScale(Point3d vs,Point3d center)
{
  auto lcs=dragging.type? dragging.lcs   : this->lcs;
  auto obj=dragging.type? dragging.begin : this->obj;

  center=lcs.getPointRelativeToCenter(center);

  auto T= Matrix::translate(+center) *
    Matrix::scaleAroundAxis(lcs.getAxis(2).normalized(),vs[2]) *
    Matrix::scaleAroundAxis(lcs.getAxis(1).normalized(),vs[1]) *
    Matrix::scaleAroundAxis(lcs.getAxis(0).normalized(),vs[0]) *
    Matrix::translate(-center);

  setObject(Position(T,obj),true);

  if (dragging.type)
    this->lcs=LocalCoordinateSystem(T,dragging.lcs);
}


////////////////////////////////////////////////////////////////////////////
void FreeTransform::glMousePressEvent(const FrustumMap& map,QMouseEvent* evt)
{
  if (evt->button()!=Qt::LeftButton) 
    return;

  Point2d pos(evt->x(),evt->y());

  //scaling
  if (!dragging.type)
  {
    for (int A=0;A<3;A++)
    {
      if (canScale(A)) 
      { 
        Point3d axis_p1_onscreen=map.applyDirectMap(Point4d(lcs.getMinAxisPoint(A),1.0)).dropHomogeneousCoordinate().toPoint3(); axis_p1_onscreen[2]=0;
        Point3d axis_p2_onscreen=map.applyDirectMap(Point4d(lcs.getMaxAxisPoint(A),1.0)).dropHomogeneousCoordinate().toPoint3(); axis_p2_onscreen[2]=0;
        Segment axis_onscreen=Segment(axis_p1_onscreen,axis_p2_onscreen);
        bool p1_near=NearBy(axis_onscreen.p1.toPoint2(),pos);
        bool p2_near=NearBy(axis_onscreen.p2.toPoint2(),pos);
        if (p1_near || p2_near)
        {
          //ambiguity
          if (dragging.type || (p1_near && p2_near)) 
          {
            dragging.type=NoDragging;
            return;
          }

          dragging.type=Scaling;
          dragging.begin=this->obj;
          dragging.axis=A;
          dragging.axis_onscreen=axis_onscreen;
          dragging.p0=2*dragging.axis_onscreen.getPointProjection(Point3d(pos))-1; //from range [0,1] to range [-1,+1]
          dragging.vs=Point3d(1,1,1);
          dragging.vs_center=Point3d(0,0,0);
          dragging.lcs=lcs;
        }
      }
    }
  }

  //translate
  if (!dragging.type)
  {
    for (int A=0;A<3;A++)
    {
      if (canTranslate(A)) 
      {
        Point3d axis_p1_onscreen=map.applyDirectMap(Point4d(lcs.getMinAxisPoint(A),1.0)).dropHomogeneousCoordinate().toPoint3(); axis_p1_onscreen[2]=0;
        Point3d axis_p2_onscreen=map.applyDirectMap(Point4d(lcs.getMaxAxisPoint(A),1.0)).dropHomogeneousCoordinate().toPoint3(); axis_p2_onscreen[2]=0;
        Segment axis_onscreen=Segment(axis_p1_onscreen,axis_p2_onscreen);

        double p0=axis_onscreen.getPointProjection(Point3d(pos)); //range is [0,1]
        if (p0>0 && p0<1 && NearBy(axis_onscreen.getPoint(p0).toPoint2(),pos))
        {
          //ambiguity
          if (this->dragging.type) 
          {
            this->dragging.type=NoDragging;
            return;
          } 

          dragging.type           = Translating;
          dragging.begin          = this->obj;
          dragging.axis           = A;
          dragging.axis_onscreen  = axis_onscreen;
          dragging.p0             = 2*p0-1; //p0 has range [0,1], dragging.range has range [-1,+1]
          dragging.vt             = Point3d(1,1,1);
          dragging.lcs=lcs;
        }
      }
    }
  }

  //rotate
  if (!dragging.type)
  {
    for (int A=0;A<3;A++)
    {
      if (this->canRotate(A)) 
      {
        Frustum frustum=map.getFrustum();
        frustum.multModelview(Matrix(lcs.getXAxis(),lcs.getYAxis(),lcs.getZAxis(),lcs.getCenter()));
        FrustumMap unit_map(frustum);

        auto ray=unit_map.getRay(pos);
        if (!ray.valid()) continue;

        Point3d circle_point=RayCircleDistance(ray,Circle(Point3d(),1.0,Point3d(0,0,0).set(A,1))).closest_circle_point.toPoint3();
        Point2d circle_point_onscreen=unit_map.projectPoint(circle_point); 
        if (NearBy(circle_point_onscreen,pos))
        {
          //ambiguity
          if (this->dragging.type) 
          {
            this->dragging.type=NoDragging;
            return;
          } 

          dragging.type = Rotating;
          dragging.begin=this->obj;
          dragging.axis=A;
          if      (A==0) dragging.p0=atan2(circle_point[2],circle_point[1]);
          else if (A==1) dragging.p0=atan2(circle_point[0],circle_point[2]);
          else if (A==2) dragging.p0=atan2(circle_point[1],circle_point[0]);
          dragging.vr=Point3d(0,0,0);
          dragging.lcs=lcs;
        }
      }
    }
  }

  if (!dragging.type)
    return;

  evt->accept();
}
  
////////////////////////////////////////////////////////////////////////////
void FreeTransform::glMouseMoveEvent(const FrustumMap& map,QMouseEvent* evt)
{
  if (!(evt->buttons() & Qt::LeftButton) )
    return;

  Point2d pos(evt->x(),evt->y());

  switch (dragging.type)
  {
    case NoDragging:
      return;
      
    case Translating: 
    {
      double p1 = 2*dragging.axis_onscreen.getPointProjection(Point3d(pos))-1; //t1 has range [-1,+1] 
      dragging.vt=Point3d(0,0,0).set(dragging.axis,(p1-dragging.p0) * dragging.lcs.getAxis(dragging.axis).module());
      doTranslate(dragging.vt);
      break;
    }

    case Rotating   : 
    {
      //I'm working in unit_system
      Frustum frustum=map.getFrustum();
      frustum.multModelview(Matrix(dragging.lcs.getXAxis(),dragging.lcs.getYAxis(),dragging.lcs.getZAxis(),dragging.lcs.getCenter()));
      FrustumMap unit_map(frustum);

      auto ray=unit_map.getRay(pos);
      if (!ray.valid()) return ;

      Point3d circle_point=RayCircleDistance(ray,Circle(Point3d(),1,Point3d(0,0,0).set(dragging.axis,1))).closest_circle_point.toPoint3();

      double p1=dragging.p0;
      if      (dragging.axis==0)  p1=atan2(circle_point[2],circle_point[1]); 
      else if (dragging.axis==1)  p1=atan2(circle_point[0],circle_point[2]);
      else if (dragging.axis==2)  p1=atan2(circle_point[1],circle_point[0]);
      dragging.vr=Point3d(0,0,0).set(dragging.axis,p1-dragging.p0);
      doRotate(dragging.vr);
      break;
    }

    case Scaling:
    {
      double p1=2*dragging.axis_onscreen.getPointProjection(Point3d(pos))-1; //[-1,+1]

      //I want the opposite side to be fixed (vs_center is the point fixed)
      if (bool bOppositeSideNotMoving=(QApplication::keyboardModifiers() & Qt::ShiftModifier))
      {
        double opposite_side=(dragging.p0 > 0) ? -1:+1; 
        dragging.vs=Point3d(1,1,1).set(dragging.axis,(p1-opposite_side)/(dragging.p0-opposite_side));
        dragging.vs_center=Point3d(0,0,0).set(dragging.axis,opposite_side);
        doScale(dragging.vs,dragging.vs_center);
      }
      //uniform scaling
      else if (bool bUniformScaling=(QApplication::keyboardModifiers() & Qt::ControlModifier))
      {
        double factor=p1/dragging.p0;
        dragging.vs=Point3d(canScale(0)?factor:1.0,canScale(1)?factor:1.0,canScale(2)?factor:1.0);
        doScale(dragging.vs);

      }
      //scale along center
      else
      {
        dragging.vs=Point3d(1,1,1).set(dragging.axis,p1/dragging.p0);
        doScale(dragging.vs);
      }

      break;
    }
  }
  
  evt->accept(); 
}
  
////////////////////////////////////////////////////////////////////////////
void FreeTransform::glMouseReleaseEvent(const FrustumMap& map,QMouseEvent* evt)
{
  if (evt->button()!=Qt::LeftButton || !dragging.type) 
    return;

  dragging.type=NoDragging;
  setObject(obj,true);
  evt->accept();  
}


//////////////////////////////////////////////////////////////////////////////
void FreeTransform::glRenderTranslate(GLCanvas& gl) 
{
  Color axis_color[3];
  axis_color[0]=Colors::Red   .withAlpha(0.8f);
  axis_color[1]=Colors::Green .withAlpha(0.8f);
  axis_color[2]=Colors::Blue  .withAlpha(0.8f);

  int line_width[3];
  line_width[0]=this->canTranslate(0)?1:0;
  line_width[1]=this->canTranslate(1)?1:0;
  line_width[2]=this->canTranslate(2)?1:0;

  if (dragging.type==Translating)
  {
    int axis=dragging.axis;
    axis_color[axis]=Colors::Yellow.withAlpha(0.8f);
    line_width[axis]=3;
  }

  for (int A=0;A<3;A++)
    GLLine(lcs.getMinAxisPoint(A),lcs.getMaxAxisPoint(A),axis_color[A],line_width[A]).glRender(gl);  
}

//////////////////////////////////////////////////////////////////////////////
void FreeTransform::glRenderRotate(GLCanvas& gl) 
{
  int line_width[3];
  line_width[0]=this->canRotate(0)? 1 : 0;
  line_width[1]=this->canRotate(1)? 1 : 0;
  line_width[2]=this->canRotate(2)? 1 : 0;

  Color line_color[3];
  line_color[0]=Colors::Red   .withAlpha(0.8f);
  line_color[1]=Colors::Green .withAlpha(0.8f);
  line_color[2]=Colors::Blue  .withAlpha(0.8f);

  if (dragging.type==Rotating)
  {
    int axis=dragging.axis;
    line_width[axis]=3;
    line_color[axis]=Colors::Yellow.withAlpha(0.8f);
  }

  for (int A=0;A<3;A++)
    GLLine(lcs.getMinAxisPoint(A),lcs.getMaxAxisPoint(A),line_color[A],line_width[A]).glRender(gl);

  const int circle_rotation[3]={1,0,2};
  for (int A=0;A<3;A++)
  {
    if (!line_width[A]) 
      continue; 

    gl.pushModelview();
    gl.multModelview(Matrix(lcs.getXAxis(),lcs.getYAxis(),lcs.getZAxis(),lcs.getCenter()));
    gl.multModelview(Matrix::rotateAroundAxis(Point3d(0,0,0).set(circle_rotation[A],1),Math::Pi/2));
    GLWireCircle(1.0,Point2d(),line_color[A],line_width[A]).glRender(gl);
    gl.popModelview();
  }  
}

///////////////////////////////////////////////////////////////////////////////
void FreeTransform::glRenderScale(GLCanvas& gl) 
{
  int line_width[3];
  line_width[0]=this->canScale(0)? 1 : 0;
  line_width[1]=this->canScale(1)? 1 : 0;
  line_width[2]=this->canScale(2)? 1 : 0;

  Color line_color[3];
  line_color[0]=Colors::Red   .withAlpha(0.8f);
  line_color[1]=Colors::Green .withAlpha(0.8f);
  line_color[2]=Colors::Blue  .withAlpha(0.8f);

  double screen_tolerance=6.0;
  double magnet_radius[3];
  magnet_radius[0]=1.0*screen_tolerance;
  magnet_radius[1]=1.0*screen_tolerance;
  magnet_radius[2]=1.0*screen_tolerance;

  if (dragging.type==Scaling)
  {
    int axis=dragging.axis;
    line_width[axis]=3;
    line_color[axis]=(Colors::Yellow).withAlpha(0.8f).withAlpha(0.5f);
    magnet_radius[axis]=1.2*screen_tolerance;
  }

  FrustumMap map(gl.getFrustum());

  gl.pushFrustum();
  gl.setHud();
  gl.pushDepthTest(false);
  gl.pushBlend(true);

  for (int A=0;A<3;A++)
  {
    if (!line_width[A]) 
      continue;
        
    for (int I=-1;I<=+1;I+=2)
    {
      Point2d p=map.projectPoint(lcs.getAxisPoint(A,I));
      Point2d p1=p-Point2d(magnet_radius[A],magnet_radius[A]);
      Point2d p2=p+Point2d(magnet_radius[A],magnet_radius[A]);
      GLQuad(p1,p2,Colors::DarkGray.withAlpha(0.5),line_color[A]).glRender(gl);
    }
  } 

  gl.popBlend();
  gl.popDepthTest();
  gl.popFrustum();
}


////////////////////////////////////////////////////////////////////////////
void FreeTransform::glRender(GLCanvas& gl)
{
  gl.pushBlend(true);
  gl.pushDepthTest(false);

  glRenderTranslate(gl);
  glRenderRotate(gl);
  glRenderScale(gl);

  gl.popDepthTest();
  gl.popBlend();
}



} //namespace Visus



