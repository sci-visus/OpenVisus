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

#if !SWIG
extern VISUS_KERNEL_API std::function<void(const String & msg)> RedirectLog;
#endif

enum 
{
  DebugPrintLevel   = 0,
  InfoPrintLevel    = 1,
  WarningPrintLevel = 2,
  ErrorPrintLevel   = 3
};

VISUS_KERNEL_API void PrintEx(String file, int line, int level, String msg);

#define PrintInfo(...) PrintEx(__FILE__,__LINE__,InfoPrintLevel, cstring(__VA_ARGS__))

/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API LogFormattedMessage 
{
public:

  String     file;
  int        line = 0;
  int        level;

  //constructor
  LogFormattedMessage(String file_ = "", int line_ = 0, int level_ = InfoPrintLevel)
    : file(file_), line(line_), level(level_){
  }

  //destructor
  ~LogFormattedMessage() {
    PrintEx(file, line, level, out.str());
  }

  //get_stream
  std::ostringstream& get_stream() {
    return out;
  }

private:

#if !SWIG
  std::ostringstream out;
#endif
};

#define VisusDebug()    (Visus::LogFormattedMessage(__FILE__,__LINE__,DebugPrintLevel  ).get_stream())
#define VisusInfo()     (Visus::LogFormattedMessage(__FILE__,__LINE__,InfoPrintLevel   ).get_stream()) 
#define VisusWarning()  (Visus::LogFormattedMessage(__FILE__,__LINE__,WarningPrintLevel).get_stream())
#define VisusError()    (Visus::LogFormattedMessage(__FILE__,__LINE__,ErrorPrintLevel  ).get_stream())


} //namespace Visus

#endif //_VISUS_LOG_H__
