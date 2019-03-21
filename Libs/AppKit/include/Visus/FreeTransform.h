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

#ifndef VISUS_FREE_TRANSFORM_H__
#define VISUS_FREE_TRANSFORM_H__

#include <Visus/AppKit.h>
#include <Visus/Matrix.h>
#include <Visus/LocalCoordinateSystem.h>
#include <Visus/Position.h>
#include <Visus/GLCanvas.h>
#include <Visus/GuiFactory.h>
#include <Visus/GLObjects.h>


namespace Visus {


//////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API FreeTransform : 
  public Model,
  public GLObject
{
public:

  VISUS_NON_COPYABLE_CLASS(FreeTransform)

  enum DraggingType 
  {
    NoDragging=0,
    Translating,
    Rotating,
    Scaling
  };

  //________________________________________________
  class VISUS_APPKIT_API Dragging
  {
  public:

    DraggingType            type=NoDragging;
    Position                begin;
    int                     axis=0;
    Point3d                 vt,vr,vs=Point3d(1,1,1),vs_center=Point3d(0,0,0);
    Segment                 axis_onscreen;
    double                  p0=0;
    LocalCoordinateSystem   lcs;
  };

  Signal<void(Position)> objectChanged;

  //constructor
  FreeTransform() {
  }

  //destructor
  virtual ~FreeTransform() {
  }

  //getTypeName
  virtual String getTypeName() const override{
    return "FreeTransform";
  }

  //getObject
  const Position& getObject() const {
    return this->obj;
  }

  //setObject
  bool setObject(const Position& obj,bool bEmitSignal=false);

  //getDragging
  const Dragging& getDragging() const {
    return dragging;
  }

  //canTranslate
  bool canTranslate(int axis) const {
    return this->obj.valid();
  }

  //canRotate
  bool canRotate(int axis) const {
    Box3d box=this->obj.getBox();

    return this->obj.valid() 
      && !(box.p1[(axis+1)%3]==box.p2[(axis+1)%3] && box.p1[(axis+2)%3]==box.p2[(axis+2)%3]);
  }

  //canScale
  bool canScale(int axis) const {
    Box3d box=this->obj.getBox();
    return this->obj.valid() 
      && !(box.p1[axis]==box.p2[axis]);
  }

  //mouse events
  virtual void glMousePressEvent  (const FrustumMap&,QMouseEvent* evt) override;
  virtual void glMouseMoveEvent   (const FrustumMap&,QMouseEvent* evt) override;
  virtual void glMouseReleaseEvent(const FrustumMap&,QMouseEvent* evt) override;

  //doTranslate
  void doTranslate(Point3d vt);

  //doRotate
  void doRotate(Point3d vr);

  //doScale
  void doScale(Point3d vs,Point3d center=Point3d());

  //glRender
  virtual void glRender(GLCanvas& gl) override;

  //writeToObjectStream (to implement if needed!)
  virtual void writeToObjectStream(ObjectStream& ostream) override {
    VisusAssert(false);
  }

  //readFromObjectStream (to implement if needed!)
  virtual void readFromObjectStream(ObjectStream& istream) override {
    VisusAssert(false);
  }

private:

  Position              obj;
  LocalCoordinateSystem lcs;
  Dragging              dragging;

  //glRenderTranslate
  void glRenderTranslate(GLCanvas& gl);

  //glRenderRotate
  void glRenderRotate(GLCanvas& gl);

  ///glRenderScale
  void glRenderScale(GLCanvas& gl);

}; //end class



} //namespace Visus

#endif //VISUS_FREE_TRANSFORM_H__

