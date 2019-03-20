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

#ifndef VISUS_OBJECT_STREAM_H__
#define VISUS_OBJECT_STREAM_H__

#include <Visus/Kernel.h>
#include <Visus/StringMap.h>
#include <Visus/Singleton.h>

#include <stack>
#include <vector>
#include <iostream>

namespace Visus {

//predeclaration
class StringTree;

  /////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ObjectStream 
{
public:

  VISUS_CLASS(ObjectStream)

  StringMap run_time_options;

  //constructor
  ObjectStream() : mode(0) {
  }

  //constructor
  ObjectStream(StringTree& root_,int mode_) : mode(0) {
    open(root_,mode_);
  }

  //destructor
  virtual ~ObjectStream(){
    close();
  }

  //open
  void open(StringTree& root,int mode);

  //close
  void close();

  //getCurrentDepth
  int getCurrentDepth(){
    return (int)stack.size();
  }

  //getCurrentContext
  StringTree* getCurrentContext(){
    return stack.top().context;
  }

  // pushContext
  bool pushContext(String context_name);

  //popContext
  bool popContext(String context_name);

  //writeInline
  void writeInline(String name, String value);

  //readInline
  String readInline(String name, String default_value = "");

  //write
  void write(String name, String value);

  //read
  String read(String name, String default_value = "");

  //writeText
  void writeText(const String& value, bool bCData = false);

  //readText
  String readText();

public:

  //setSceneMode
  void setSceneMode(bool value) {
    run_time_options.setValue("scene_mode",cstring(value));
  }

  //isSceneMode
  bool isSceneMode() const {
    return cbool(run_time_options.getValue("scene_mode"));
  }

private:

  int mode;

  class StackItem
  {
  public:
    StringTree*                  context;
    std::map<String,StringTree*> next_child;
    StackItem(StringTree* context_=nullptr) : context(context_) {}
  };
  std::stack<StackItem> stack;

};

  
} //namespace Visus


#endif //VISUS_OBJECT_STREAM_H__


