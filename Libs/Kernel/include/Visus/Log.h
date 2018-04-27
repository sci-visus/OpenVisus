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

#ifndef _VISUS_LOG_H__
#define _VISUS_LOG_H__

#include <Visus/Kernel.h>
#include <Visus/Time.h>

#include <iostream>
#include <functional>
#include <fstream>

namespace Visus {

/////////////////////////////////////////////////////////////////
#if SWIG
class VISUS_KERNEL_API Log
{
public:
  static void printMessage(String s);

private:
  Log();
};

#else

class VISUS_KERNEL_API Log  
{
public:

  VISUS_NON_COPYABLE_CLASS(Log)

  enum Level
  {
    Error=1, 
    Warning, 
    Info, 
    Debug
  };

  class Message
  {
  public:
    String file;
    int    line=0;
    int    level=Debug;
    Time   time;
    String content;

    //toString
    String toString() const;

  };

  static String filename;
  static std::ofstream file;
  static std::function<void(const Message& msg)> redirect;

  //constructor
  Log(String file, int line,int level);

  //destructor
  ~Log();

  //get
  std::ostringstream& get() {
    return out;
  }
  
  //printMessage
  static void printMessage(const Message& msg);

  //printMessage
  static void printMessage(String s) {
    Message msg;
    msg.file = "";
    msg.line = -1;
    msg.level = Info;
    msg.content = s;
    msg.time = Time::now();
    return printMessage(msg);
  }

private:

  std::ostringstream out;

  Message msg;

};

#define VisusError()    (Visus::Log(__FILE__,__LINE__,Visus::Log::Error  ).get())
#define VisusWarning()  (Visus::Log(__FILE__,__LINE__,Visus::Log::Warning).get())
#define VisusInfo()     (Visus::Log(__FILE__,__LINE__,Visus::Log::Info   ).get()) 
#define VisusDebug()    (Visus::Log(__FILE__,__LINE__,Visus::Log::Debug  ).get())

#endif //SWIG

} //namespace Visus

#endif //_VISUS_LOG_H__
