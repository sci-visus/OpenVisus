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

#ifndef _VISUS_CANVAS_H__
#define _VISUS_CANVAS_H__

#include <Visus/Gui.h>
#include <Visus/Utils.h>

#include <QFrame>
#include <QMouseEvent>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API Canvas : public QFrame
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(Canvas)

  //constructor
  Canvas() {
    setMouseTracking(true);
  }

  //destructor
  virtual ~Canvas() {
  }
  
  //renderBackground
  void renderBackground(QPainter& painter)
  {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(200,200,230));
    painter.drawRect(0,0,this->width()-1,this->height()-1);
    painter.setBrush(Qt::white);
    painter.drawRect(project(QRectF(world_bounds.x,world_bounds.y,world_bounds.width,world_bounds.height)));
  }

  //renderGrid
  void renderGrid(QPainter& painter,int num_x=32,int num_y=32)
  {
    for (int I=0;I<=num_x;I++)
    {
      auto x=world_bounds.x+(I/(double)num_x)*world_bounds.width;
      painter.setPen(QColor(173,216,230,50));
      painter.drawLine(project(QPointF(x,0)), project(QPointF(x,1)));
    }

    for (int I=0;I<=num_y;I++)
    {
      auto y=world_bounds.y+(I/(double)num_y)*world_bounds.height;
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
    painter.drawRect(project(QRectF(world_bounds.x,world_bounds.y,world_bounds.width,world_bounds.height))); 
  }

  //getWorldBounds
  Rectangle2d getWorldBounds() const {
    return world_bounds;
  }

  //setWorldBounds
  void setWorldBounds(Rectangle2d world_bounds)
  {
    this->world_bounds=world_bounds;
    this->world_pos=world_bounds.p1();

    this->Tproject=
      Matrix3::scale(Point2d(1.0/world_bounds.width,1.0/world_bounds.height)) *
      Matrix3::translate(Point2d(-world_bounds.x,-world_bounds.y));
    this->Tunproject=Tproject.invert();
    
    update();
    emit repaintNeeded();
  }

  //setWorldBounds
  void setWorldBounds(double x,double y,double width,double height) {
    setWorldBounds(Rectangle2d(x,y,width,height));
  }

  //getOrthoParams
  Matrix3 getOrthoParams() const {
    return Tproject;
  }

  //setOrthoParams
  void setOrthoParams(Matrix3 value) {
    Tproject=value;
    emit repaintNeeded();
  }

  //project
  Point2d project(Point2d pos) const {
    auto eye=(Tproject * Point3d(pos,1)).dropZ();
    auto screenpos=Point2d(this->width()*eye.x,this->height()*eye.y);
    screenpos.y=this->height()-screenpos.y-1; //mirror y
    return screenpos;
  }

  //unproject
  Point2d unproject(Point2d screenpos) const {
    screenpos.y=this->height()-screenpos.y-1; //mirror y
    auto eye=Point2d(screenpos.x/(double)this->width(),screenpos.y/(double)this->height());
    return (Tunproject * Point3d(eye,1)).dropZ();
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
    return world_pos;
  }

  //setCurrentPos
  void setCurrentPos(Point2d value) {
    this->world_pos=value;
    update();
    emit repaintNeeded();
  }

  //enterEvent
  virtual void enterEvent(QEvent* evt) override {
    setFocus();
    update();
    emit repaintNeeded();
  }

  //leaveEvent
  virtual void leaveEvent(QEvent* evt) override {
    clearFocus();
    update();
    emit repaintNeeded();
  }

  //resizeEvent
  virtual void resizeEvent(QResizeEvent* evt) override {
    setWorldBounds(getWorldBounds());
  }

  //mouseDoubleClickEvent
  virtual void mouseDoubleClickEvent(QMouseEvent * e) override {
    if (e->button()==Qt::RightButton)
      setWorldBounds(getWorldBounds());
  }

  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override 
  {
    if (evt->button()==Qt::RightButton)
    {
      bPanning=true;
      evt->accept();
    }
    
    world_pos  = QUtils::convert<Point2d>(unproject(evt->pos()));
    update();
    emit repaintNeeded();
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override  
  {
    if (bPanning)
    {
      auto w0=world_pos;
      auto w1=QUtils::convert<Point2d>(unproject(evt->pos()));
      Tproject     = Tproject * Matrix3::translate(w1-w0);
      Tunproject = Tproject.invert();
      evt->accept();
    }
    
    world_pos = QUtils::convert<Point2d>(unproject(evt->pos()));
    update();
    emit repaintNeeded();
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override
  {
    if (bPanning)
    {
      bPanning=false;
      evt->accept();
    }
    
    world_pos  = QUtils::convert<Point2d>(unproject(evt->pos()));
    update();
    emit repaintNeeded();
  }

  //wheelEvent
  virtual void wheelEvent(QWheelEvent* evt) override
  {
    auto vs=1+0.2*(evt->delta()/120);
    auto c0=world_pos;
    Tproject   = Tproject * Matrix3::scaleAroundCenter(c0,vs);
    Tunproject = Tproject.invert();
    update();
    emit repaintNeeded();
  }

signals:

  //repaintNeeded
  void repaintNeeded();

private:

  Matrix3     Tproject, Tunproject;
  //modelview is always identity
  Rectangle2d world_bounds=Rectangle2d(0,0,1,1);
  Point2d     world_pos;
  bool        bPanning=false;

};

} //namespace Visus


#endif //_VISUS_CANVAS_H__

