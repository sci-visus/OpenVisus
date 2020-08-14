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

#include <Visus/Gui.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
#include <Visus/Array.h>
#include <Visus/Model.h>
#include <Visus/GuiFactory.h>
#include <Visus/GLObject.h>

#include <QTimer>


namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_GUI_API ScriptingNode : public Node
  , public GLObject
{
public:

  VISUS_NON_COPYABLE_CLASS(ScriptingNode)

  //constructor
  ScriptingNode()
  {
    addInputPort("array");
    addOutputPort("array");

    setCode("output=input");
  }

  //destructor
  virtual ~ScriptingNode() {
  }

  //getTypeName
  virtual String getTypeName() const override {
    return "ScriptingNode";
  }


  //getCode
  String getCode() {
    return code;
  }

  //setCode
  void setCode(String code, bool bForce=false) {
    setProperty("SetCode", this->code, code, bForce);
  }

  //processInput
  virtual bool processInput() override
  {
    abortProcessing();

    //I'm using the shared engine...
    joinProcessing();

    // important to do before readValue
    auto return_receipt = createPassThroughtReceipt();
    auto input = readValue<Array>("array");
    if (!input)
      return  false;

    this->node_bounds = input->bounds;

    DataflowMessage msg;
    msg.setReturnReceipt(return_receipt);
    msg.writeValue("array", input);
    this->publish(msg);
    return true;
  }

  //getBounds
  virtual Position getBounds() override {
    return this->node_bounds;
  }

  //setBounds
  virtual void setBounds(Position value) {
    this->node_bounds = value;
  }

  static ScriptingNode* castFrom(Node* obj) {
    return dynamic_cast<ScriptingNode*>(obj);
  }

  //modelChanged
  virtual void modelChanged() override {
    if (dataflow)
      dataflow->needProcessInput(this);
  }

  //createEditor
  virtual void createEditor() {
  }

public:

  //replicate most of the API of GLOBject to expose them to python

  //glGetRenderQueue
  virtual int glGetRenderQueue() const override {
    return GLObject::glGetRenderQueue();
  }

  //glSetRenderQueue
  virtual void glSetRenderQueue(int value) override {
    GLObject::glSetRenderQueue(value);
  }

  //glMousePressEvent
  virtual void glMousePressEvent(const FrustumMap& map, QMouseEvent* evt) override {
    GLObject::glMousePressEvent(map, evt);
  }

  //mouseMoveEvent
  virtual void glMouseMoveEvent(const FrustumMap& map, QMouseEvent* evt) override {
    GLObject::glMouseMoveEvent(map, evt);
  }

  //mouseReleaseEvent
  virtual void glMouseReleaseEvent(const FrustumMap& map, QMouseEvent* evt) override {
    GLObject::glMouseReleaseEvent(map, evt);
  }

  //wheelEvent
  virtual void glWheelEvent(const FrustumMap& map, QWheelEvent* evt) override {
    GLObject::glWheelEvent(map, evt);
  }

  //glRender
  virtual void glRender(GLCanvas& gl) override {
    //implement your own method here
  }

public:

  //execute
  virtual void execute(Archive& ar) override
  {
    if (ar.name == "SetCode") {
      String value;
      ar.read("value", value);
      setCode(value);
      return;
    }

    return Node::execute(ar);
  }

  //write
  virtual void write(Archive& ar) const override
  {
      Node::write(ar);
      ar.writeText("code", code);
  }

  //read
  virtual void read(Archive& ar) override
  {
      Node::read(ar);
      ar.readText("code", code);
  }

private:

  String    code;
  Position  node_bounds;
};


} //namespace Visus

#endif //VISUS_SCRIPTING_NODE_H

