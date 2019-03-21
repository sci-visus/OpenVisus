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

#ifndef VISUS_GL_LOOKAT_CAMERA_H
#define VISUS_GL_LOOKAT_CAMERA_H

#include <Visus/GLCamera.h>
#include <Visus/GLMouse.h>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLookAtCamera : public GLCamera
{
public:

  VISUS_NON_COPYABLE_CLASS(GLLookAtCamera)

  //constructor
  GLLookAtCamera(double defaultRotFactor_=5.2,double defaultPanFactor_=30.0) 
    : defaultRotFactor(defaultRotFactor_),defaultPanFactor(defaultPanFactor_)
  {
    last_mouse_pos.resize(GLMouse::getNumberOfButtons());
  }

  //destructor
  virtual ~GLLookAtCamera() {
  }

  //getTypeName
  virtual String getTypeName() const override {
    return "GLLookAtCamera";
  }

  //getCenterOfRotation
  Point3d getCenterOfRotation() const {
    return centerOfRotation;
  }

  //set center of rotation
  void setCenterOfRotation(Point3d value);

  //isUsingOrthoProjection
  inline bool isUsingOrthoProjection() const {
    return bUseOrthoProjection;
  }

  //setUseOrthoProjection
  void setUseOrthoProjection(bool value);

  //isAutoOrthoParams
  bool isAutoOrthoParams() const {
    return bAutoOrthoParams;
  }

  //setAutoOrthoParams
  void setAutoOrthoParams(bool value) ;

  //getOrthoParams
  virtual GLOrthoParams getOrthoParams() const override {
     return ortho_params; 
  }

  //setOrthoParams
  virtual void setOrthoParams(GLOrthoParams value) override;

  //getViewport
  virtual Viewport getViewport() const override {
    return viewport;
  }

  //setViewport
  virtual void setViewport(Viewport value) override;

  //getFrustum
  virtual Frustum getFrustum() const override;

  //glMousePressEvent
  virtual void glMousePressEvent(QMouseEvent* evt) override;

  //glMouseMoveEvent
  virtual void glMouseMoveEvent(QMouseEvent* evt) override;

  //glMouseReleaseEvent
  virtual void glMouseReleaseEvent (QMouseEvent* evt) override;

  //glWheelEvent
  virtual void glWheelEvent(QWheelEvent* evt) override;

  //glKeyPressEvent
  virtual void glKeyPressEvent(QKeyEvent* evt) override;

  //guessPosition
  virtual bool guessPosition(Position position,int ref=-1) override;

  //setBounds
  void setBounds(Box3d box);

  //setLookAt
  void setLookAt(Point3d pos,Point3d center,Point3d vup);

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  Box3d                  bound;
  bool                 bUseOrthoProjection=false;
  double               defaultRotFactor;
  double               defaultPanFactor;
  bool                 bDisableRotation=false;
  bool                 bAutoOrthoParams=true;
  std::vector<Point2i> last_mouse_pos;
  GLMouse              mouse;
  Viewport             viewport;
  GLOrthoParams        ortho_params;

  //modelview
  Point3d      pos,dir,vup;
  Quaternion4d quaternion;
  Point3d      centerOfRotation;

  //forward
  void forward(double factor);

  //pan
  void pan(Point2d screen_p1,Point2d screen_p2);

  //rotate
  void rotate(Point2d screen_center,double angle);

  //guessNearFarDistance
  std::pair<double,double> guessNearFarDistance() const;

  //guessForwardFactor
  double guessForwardFactor() const ;

  //guessOrthoParams
  void guessOrthoParams();


};//end class

} //namespace Visus

#endif //VISUS_GL_LOOKAT_CAMERA_H

