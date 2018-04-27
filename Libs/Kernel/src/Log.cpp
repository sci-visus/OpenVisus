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

#include <iostream>
#include <cctype>
#include <iomanip>

#if WIN32
#include <Windows.h>
#endif

namespace Visus {

String Log::filename;
std::ofstream Log::file;
std::function<void(const Log::Message& msg)> Log::redirect;

////////////////////////////////////////////////////////////////////
String Log::Message::toString() const {

  std::ostringstream out;
  out
    <<std::setfill('0')
    <<std::setw(2)<<(time.getYear()-2000)<<"."
    <<std::setw(2)<<(time.getMonth()+1)<<"."
    <<std::setw(2)<<(time.getDayOfMonth())<<"|"
    <<std::setw(2)<<(time.getHours())<<"."
    <<std::setw(2)<<(time.getMinutes())<<"."
    <<std::setw(2)<<(time.getSeconds())<<"."
    <<std::setw(3)<<(time.getMilliseconds());
  
  out<<" "<<file<<"("<<line<<") "<<content<<std::endl;
  return out.str();
}

////////////////////////////////////////////////////////////////////
Log::Log(String file, int line,int level) 
{
  msg.file=file;
  msg.line=line;
  msg.level=level;
  msg.time=Time::now();

  msg.file = msg.file.substr(msg.file.find_last_of("/\\") + 1);
  msg.file = msg.file.substr(0, msg.file.find_last_of('.'));
}

////////////////////////////////////////////////////////////////////
Log::~Log()
{
  msg.content=out.str();
  Log::printMessage(msg);
}

////////////////////////////////////////////////////////////////////
void Log::printMessage(const Message& msg)
{
  auto s = msg.toString();
  std::cout << s;

  //output to Visual studio console
#if WIN32
  OutputDebugStringA(s.c_str());
#endif  

  //print to file
  if (Log::file.is_open())
    Log::file << s;

  //let know to the outside
  if (Log::redirect)
    Log::redirect(msg);

}


} //namespace Visus

