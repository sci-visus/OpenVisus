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

#ifndef _VISUS_QCANVAS_2D_H__
#define _VISUS_QCANVAS_2D_H__

#include <Visus/Gui.h>
#include <Visus/Utils.h>

#include <QFrame>
#include <QMouseEvent>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API QCanvas2d : public QFrame
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(QCanvas2d)

  //constructor
    QCanvas2d() {
    setMouseTracking(true);
  }

  //destructor
  virtual ~QCanvas2d() {
  }
  
  //renderBackground
  void renderBackground(QPainter& painter)
  {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(200,200,230));
    painter.drawRect(0,0,this->width()-1,this->height()-1);
    painter.setBrush(Qt::white);
    painter.drawRect(project(QRectF(world_box.x,world_box.y,world_box.width,world_box.height)));
  }

  //renderGrid
  void renderGrid(QPainter& painter,int num_x=32,int num_y=32)
  {
    for (int I=0;I<=num_x;I++)
    {
      auto x=world_box.x+(I/(double)num_x)*world_box.width;
      painter.setPen(QColor(173,216,230,50));
      painter.drawLine(project(QPointF(x,0)), project(QPointF(x,1)));
    }

    for (int I=0;I<=num_y;I++)
    {
      auto y=world_box.y+(I/(double)num_y)*world_box.height;
      painter.setPen(QColor(173,216,230,50));
      painter.drawLine(project(QPointF(0,y)), project(QPointF(1,y)));
    }
  }

  //renderBorders
  void renderBorders(QPainter& painter)
  {
    painter.setPen(QColor(0,0,0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0,0,this->width()-1,this->height()-1);
    painter.drawRect(project(QRectF(world_box.x,world_box.y,world_box.width,world_box.height))); 
  }

  //getWorldBox
  Rectangle2d getWorldBox() const {
    return world_box;
  }

  //setWorldBox
  void setWorldBox(Rectangle2d value)
  {
    this->world_box= value;
    this->current_pos = value.p1();

    this->Tproject=
      Matrix::scale(Point2d(1.0/ value.width,1.0/ value.height)) *
      Matrix::translate(Point2d(-value.x,-value.y));
    this->Tunproject=Tproject.invert();
    
    postRedisplay();
  }

  //setWorldBounds
  void setWorldBox(double x,double y,double width,double height) {
    setWorldBox(Rectangle2d(x,y,width,height));
  }

  //getProjection
  Matrix getProjection() const {
    return Tproject;
  }

  //setProjection
  void setProjection(Matrix value) {
    Tproject=value;
  }

  //project
  Point2d project(Point2d pos) const {
    auto eye=(Tproject * Point3d(pos,1)).toPoint2();
    auto screenpos=Point2d(this->width()*eye[0],this->height()*eye[1]);
    screenpos[1]=this->height()-screenpos[1]-1; //mirror y
    return screenpos;
  }

  //unproject
  Point2d unproject(Point2d screenpos) const {
    screenpos[1]=this->height()-screenpos[1]-1; //mirror y
    auto eye=Point2d(screenpos[0]/(double)this->width(),screenpos[1]/(double)this->height());
    return (Tunproject * Point3d(eye,1)).toPoint2();
  }

  //project
  QPointF project(QPointF pos) const {
    return QUtils::convert<QPointF>(project(QUtils::convert<Point2d>(pos)));
  }

  //project
  QRectF project(QRectF r) const {
    auto s0=project(r.bottomLeft());
    auto s1=project(r.topRight  ());
    return QRectF(std::min(s0.x(),s1.x()),std::min(s0.y(),s1.y()),fabs(s1.x()-s0.x()),fabs(s1.y()-s0.y()));
  }

  //unproject
  QPointF unproject(QPointF screenpos) const {
    return QUtils::convert<QPointF>(unproject(QUtils::convert<Point2d>(screenpos)));
  }

  //heightForWidth
  virtual QSize	sizeHint() const override{
    int width=480;
    return QSize(width,heightForWidth(width));
  }

  //heightForWidth
  virtual int heightForWidth(int width) const override {
    return (int)(width*0.5);
  }

  //getCurrentPos
  Point2d getCurrentPos() const {
    return current_pos;
  }

  //setCurrentPos
  void setCurrentPos(Point2d value) {
    this->current_pos =value;
    postRedisplay();
  }

  //enterEvent
  virtual void enterEvent(QEvent* evt) override {
    setFocus();
    postRedisplay();
  }

  //leaveEvent
  virtual void leaveEvent(QEvent* evt) override {
    clearFocus();
    postRedisplay();
  }

  //resizeEvent
  virtual void resizeEvent(QResizeEvent* evt) override {
    setWorldBox(getWorldBox());
  }

  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override 
  {
    if (evt->button()==Qt::RightButton)
      bPanning=true;
    
    current_pos = QUtils::convert<Point2d>(unproject(evt->pos()));
    postRedisplay();
    evt->accept();
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override  
  {
    if (bPanning)
    {
      auto w0= current_pos;
      auto w1=QUtils::convert<Point2d>(unproject(evt->pos()));
      Tproject     = Tproject * Matrix::translate(w1-w0);
      Tunproject = Tproject.invert();
    }
    
    current_pos = QUtils::convert<Point2d>(unproject(evt->pos()));
    postRedisplay();
    evt->accept();
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override
  {
    if (bPanning)
      bPanning=false;
    
    current_pos = QUtils::convert<Point2d>(unproject(evt->pos()));
    postRedisplay();
    evt->accept();
  }

  //wheelEvent
  virtual void wheelEvent(QWheelEvent* evt) override
  {
    auto vs=1+0.2*(evt->delta()/120);
    auto c0= current_pos;
    Tproject   = Tproject * Matrix::scaleAroundCenter(c0,vs);
    Tunproject = Tproject.invert();
    postRedisplay();
    evt->accept();
  }

  //postRedisplay
  void postRedisplay() {
    update();
  }

protected:

  Matrix      Tproject =Matrix(3);
  Matrix      Tunproject = Matrix(3);
  //modelview is always identity
  Rectangle2d world_box=Rectangle2d(0,0,1,1);
  Point2d     current_pos;
  bool        bPanning=false;

};

} //namespace Visus


#endif //_VISUS_QCANVAS_2D_H__

