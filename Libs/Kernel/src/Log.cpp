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

#include <Visus/Log.h>
#include <Visus/Thread.h>
#include <Visus/Utils.h>

#include <iostream>
#include <cctype>
#include <iomanip>

#if WIN32
#include <Windows.h>
#endif

namespace Visus {

std::function<void(const String& msg)> RedirectLog;

//////////////////////////////////////////////////////////////////////////
LogFormattedMessage::LogFormattedMessage(String file_, int line_, String level_)
  : file(file_), line(line_), level(level_), time(Time::now())
{
  if (!file.empty())
  {
    this->file = this->file.substr(this->file.find_last_of("/\\") + 1);
    this->file = this->file.substr(0, this->file.find_last_of('.'));
  }

  out
    << "[" << file << ": " << line << "] [pid " << Utils::getPid() << ":tid " << Thread::getThreadId() << "] "
    << std::setfill('0')
    << "["
    //<<std::setw(2)<<(time.getYear()-2000)<<"."
    //<<std::setw(2)<<(time.getMonth()+1)<<"."
    //<<std::setw(2)<<(time.getDayOfMonth())<<"|"
    << std::setw(2) << (time.getHours()) << "."
    << std::setw(2) << (time.getMinutes()) << "."
    << std::setw(2) << (time.getSeconds()) << "."
    << std::setw(3) << (time.getMilliseconds())
    << "] ";
}

//////////////////////////////////////////////////////////////////////////
LogFormattedMessage::~LogFormattedMessage()
{
  out << std::endl;

  String msg = out.str();

  PrintMessageToTerminal(msg);

  if (Visus::RedirectLog)
    Visus::RedirectLog(msg);
}


} //namespace Visus

