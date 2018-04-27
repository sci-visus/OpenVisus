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

#ifndef __VISUS_GL_OBJECT_H
#define __VISUS_GL_OBJECT_H

#include <Visus/Gui.h>
#include <Visus/Frustum.h>

#include <QMouseEvent>

namespace Visus {

//predeclaratin
class GLCanvas;

////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLObject 
{
public:

  VISUS_CLASS(GLObject)

  //constructor
  GLObject() {
  }

  //copy constructor
  GLObject(const GLObject& other) {
    *this=other;
  }

  //destructor
  virtual ~GLObject() {
  }

  //glRender
  virtual void glRender(GLCanvas& gl)=0;

  //glGetRenderQueue
  virtual int glGetRenderQueue() const {
    return default_render_queue;
  }

  //glSetRenderQueue
  virtual void glSetRenderQueue(int value)
  {this->default_render_queue=value;}

  //operator=
  GLObject& operator=(const GLObject& other) {
    this->default_render_queue=other.default_render_queue;
    return *this;
  }

  //glMousePressEvent
  virtual void glMousePressEvent(const FrustumMap& map, QMouseEvent* evt){
  }

  //mouseMoveEvent
  virtual void glMouseMoveEvent(const FrustumMap& map, QMouseEvent* evt){
  }

  //mouseReleaseEvent
  virtual void glMouseReleaseEvent(const FrustumMap& map, QMouseEvent* evt){
  }

  //wheelEvent
  virtual void glWheelEvent(const FrustumMap& map, QWheelEvent* evt){
  }

private:

  int default_render_queue=-1;

};

} //namespace Visus


#endif //__VISUS_GL_OBJECT_H
