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
  GLLookAtCamera(double rotation_factor_=5.2,double pan_factor_=30.0)
    : rotation_factor(rotation_factor_),pan_factor(pan_factor_)
  {
    last_mouse_pos.resize(GLMouse::getNumberOfButtons());
  }

  //destructor
  virtual ~GLLookAtCamera() {
  }

  //executeAction
  virtual void executeAction(StringTree in) override;

  //getTypeName
  virtual String getTypeName() const override {
    return "GLLookAtCamera";
  }

  //isUsingOrthoProjection
  inline bool isUsingOrthoProjection() const {
    return use_ortho_projection;
  }

  //setUseOrthoProjection
  void setUseOrthoProjection(bool value);

  //isOrthoParamsFixed
  bool isOrthoParamsFixed() const {
    return ortho_params_fixed;
  }

  //setAutoOrthoParams
  void setOrthoParamsFixed(bool value) ;

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

  //getCurrentFrustum
  virtual Frustum getFinalFrustum() const override;

  //getCurrentFrustum
  virtual Frustum getCurrentFrustum() const override {
    return getFinalFrustum();
  }

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

  //setBounds
  void setBounds(BoxNd new_value);

  //guessPosition
  virtual bool guessPosition(BoxNd value,int ref=-1) override;

  //getPosition
  Point3d getPosition() const {
    return pos;
  }

  //setPosition
  void setPosition(Point3d value);

  //getDirection
  Point3d getDirection() const {
    return dir;
  }

  //setDirection
  void setDirection(Point3d value);

  //getViewUp
  Point3d getViewUp() const {
    return vup;
  }

  //setViewUp
  void setViewUp(Point3d value);

  //getRotation
  Quaternion getRotation() const {
    return rotation;
  }

  //setRotation
  void setRotation(Quaternion q);


  //getRotationCenter
  Point3d getRotationCenter() {
    return rotation_center;
  }

  //setRotationCenter
  void setRotationCenter(Point3d value);

public:

  //writeTo
  virtual void writeTo(StringTree& out) const override;

  //readFrom
  virtual void readFrom(StringTree& in) override;

private:

  BoxNd                bounds = BoxNd(3);
  bool                 use_ortho_projection=false;
  double               rotation_factor;
  double               pan_factor;
  bool                 disable_rotation=false;
  std::vector<Point2i> last_mouse_pos;
  GLMouse              mouse;
  Viewport             viewport;
  GLOrthoParams        ortho_params;
  bool                 ortho_params_fixed = false;

  //modelview
  Point3d      pos, dir, vup;

  Quaternion   rotation;
  Point3d      rotation_center;

  //guessNearFarDistance
  std::pair<double,double> guessNearFarDistance() const;

  //guessForwardFactor
  double guessForwardFactor() const ;

  //guessOrthoParams
  GLOrthoParams guessOrthoParams() const;

  //setProperty
  template <typename Value>
  void setProperty(String target_id, Value& old_value, const Value& new_value)
  {
    if (old_value == new_value) return;
    beginUpdate(
      createPassThroughAction(StringTree("set"), target_id).write("value", new_value),
      createPassThroughAction(StringTree("set"), target_id).write("value", old_value));
    {
      old_value = new_value;
      if (target_id!= "ortho_params" && !ortho_params_fixed)
        setOrthoParams(guessOrthoParams());
    }
    endUpdate();
  }




};//end class

} //namespace Visus

#endif //VISUS_GL_LOOKAT_CAMERA_H

