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

#ifndef VISUS_SCRIPTING_NODE_H
#define VISUS_SCRIPTING_NODE_H

#include <Visus/Nodes.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
#include <Visus/Array.h>
#include <Visus/PythonEngine.h>

namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_NODES_API ScriptingNode : public Node
{
public:

  VISUS_NON_COPYABLE_CLASS(ScriptingNode)


  //constructor
  ScriptingNode(String name="");

  //destructor
  virtual ~ScriptingNode();


  //getCode
  String getCode() {
    return code;
  }

  //setCode
  void setCode(String code) {
    if (this->code == code) return;
    beginUpdate();
    this->code = code;
    endUpdate();
  }

  void addUserInput(String key, Array value) {
    try {
      ScopedAcquireGil acquire_gil;
      engine->setModuleAttr(key, value);
    }
    catch (std::exception ex)
    {
      ScopedAcquireGil acquire_gil;
      engine->printMessage(ex.what());
      return;
    }
  }

  //clearPresets
  void clearPresets();

  //getPresets
  std::vector<String> getPresets() const {
    return presets.keys;
  }

  //addPreset
  void addPreset(String key, String code);

  //getPresetCode
  String getPresetCode(String key, String default_value = "") {

    int index = Utils::find(presets.keys,key);
    return index >= 0 ? presets.code[index] : default_value;
  }

  //processInput
  virtual bool processInput() override;

  //getNodeBounds
  virtual Position getNodeBounds() override {
    return this->bounds;
  }

  static ScriptingNode* castFrom(Node* obj) {
    return dynamic_cast<ScriptingNode*>(obj);
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  class MyJob;  friend class MyJob;

  String    code;

  struct
  {
    std::vector<String> keys;
    std::vector<String> code;
  }
  presets;


  DType     last_dtype;

  Position  bounds;

#if VISUS_PYTHON
  SharedPtr<PythonEngine> engine;
#endif

  //guessPresets
  void guessPresets(Array data);

  //modelChanged
  virtual void modelChanged() override {
    if (dataflow)
      dataflow->needProcessInput(this);
  }

};


////////////////////////////////////////////////////////////
class VISUS_NODES_API ScriptingNodeBaseView : public  View<ScriptingNode>
{
public:

  //constructor
  ScriptingNodeBaseView() {
  }

  //destructor
  virtual ~ScriptingNodeBaseView(){
  }

  //clearPresets
  virtual void clearPresets() = 0;

  //addPreset
  virtual void addPreset(String key, String code) = 0;

};

} //namespace Visus

#endif //VISUS_SCRIPTING_NODE_H

