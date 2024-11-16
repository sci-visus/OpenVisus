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

#ifndef __VISUS_GL_MOUSE_H
#define __VISUS_GL_MOUSE_H

#include <Visus/Gui.h>

#include <QMouseEvent>

namespace Visus {

////////////////////////////////////////////////////
class VISUS_GUI_API GLMouse 
{
public:

  VISUS_NON_COPYABLE_CLASS(GLMouse)

  //__________________________________________________________________
  class VISUS_GUI_API ButtonStatus
  {
  public:

    bool    isDown;
    Point2i pos  ;
    Point2i down ; Time down_time; 
    Point2i up   ; Time up_time;   

    //constructor
    ButtonStatus() : isDown(false)
    {}

    //toString
    String toString() const
    {
      std::ostringstream out;
      out<<"isDown("<<isDown<<") "<<"pos("<<pos<<") "<<"down("<<down<<") "<<"up("<<up<<")";
      return out.str();
    }
  };

  //constructor
  GLMouse() : ndown(0)
  {buttons.resize(getNumberOfButtons());}

  //getNumberOfButtons
  static inline int getNumberOfButtons()
  {return 10;}

  //getButton
  inline const ButtonStatus& getButton(int nbutton) const
  {return buttons[nbutton];}

  //getNumberOfButtonDown
  inline int getNumberOfButtonDown() const
  {return ndown;}

  //glMousePressEvent
  inline void glMousePressEvent(QMouseEvent* evt)
  {
    ButtonStatus& b=this->buttons[evt->button()];
    b.down_time=Time::now();
    b.pos = b.down = Point2i(evt->x(),evt->y());
    if (!b.isDown) { ++ndown; b.isDown = true; }
  }

  //glMouseMoveEvent
  inline void glMouseMoveEvent(QMouseEvent* evt)
  {
    Point2i pos(evt->x(),evt->y());
    if ((evt->buttons() & Qt::LeftButton  )) this->buttons[Qt::LeftButton  ].pos = pos;
    if ((evt->buttons() & Qt::RightButton )) this->buttons[Qt::RightButton ].pos = pos;
    if ((evt->buttons() & Qt::MiddleButton)) this->buttons[Qt::MiddleButton].pos = pos;
  }

  //glMouseReleaseEvent
  inline void glMouseReleaseEvent(QMouseEvent* evt)
  {
    ButtonStatus& b=this->buttons[evt->button()];
    b.up_time=Time::now();
    b.pos=b.up=Point2i(evt->x(),evt->y());
    if (b.isDown) { --ndown; b.isDown = false; }
  }

  //wasSingleClick (NOTE: remember to check if evt->button()==<button>)
  inline bool wasSingleClick(int button)
  {
    return button
      && (buttons[button].down- buttons[button].up).module()<10
      && (buttons[button].up_time-buttons[button].down_time)<=200;
  }

  //toString
  String toString() const 
  {
    std::ostringstream out;
    for (int I=0;I<(int)buttons.size();I++)
      out<<"button("<<I<<") " <<buttons[I].toString()<<"\n";
    return out.str();
  }

private:

  std::vector<ButtonStatus> buttons;
  int ndown;

};

} //namespace Visus

#endif //__VISUS_GL_MOUSE_H
