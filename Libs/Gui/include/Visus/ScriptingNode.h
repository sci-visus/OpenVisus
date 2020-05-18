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

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_GUI_API ScriptingNode : public Node
#if !SWIG
  , public GLObject
#endif
{
public:

  VISUS_NON_COPYABLE_CLASS(ScriptingNode)

  //constructor
  ScriptingNode();

  //destructor
  virtual ~ScriptingNode();

  //getOsDependentTypeName (virtual so that I can override it in python)
  virtual String getOsDependentTypeName() const override {
    return "ScriptingNode";
  }

  //getCode
  String getCode() {
    return code;
  }

  //getMaxPublishMSec
  int getMaxPublishMSec() const {
    return max_publish_msec;
  }

  //setMaxPublishMSec
  void setMaxPublishMSec(int value) {
    setProperty("SetMaxPublishMSec", this->max_publish_msec, value);
  }

  //setCode
  void setCode(String code) {
    setProperty("SetCode", this->code, code);
  }

  //addUserInput
  void addUserInput(String key, Array value);

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
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

#if VISUS_PYTHON
  class MyJob;  
  friend class MyJob;
#endif

  String    code;

  int max_publish_msec = 600;

  struct
  {
    std::vector<String> keys;
    std::vector<String> code;
  }
  presets;


  DType     last_dtype;

  Position  node_bounds;

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
class VISUS_GUI_API ScriptingNodeBaseView : public  View<ScriptingNode>
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


/////////////////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API ScriptingNodeView :
  public QFrame,
  public ScriptingNodeBaseView
{
public:

  VISUS_NON_COPYABLE_CLASS(ScriptingNodeView)

    class VISUS_GUI_API Widgets
  {
  public:
    QTextEdit* txtCode = nullptr;
    QTextEdit* txtOutput = nullptr;
    QToolButton* btnRun = nullptr;
    QComboBox* presets = nullptr;
  };

  Widgets widgets;

  //constructor
  ScriptingNodeView(ScriptingNode* model = nullptr);

  //destructor
  virtual ~ScriptingNodeView();

  //clearPresets
  virtual void clearPresets() override {
    widgets.presets->clear();
  }

  //addPreset
  virtual void addPreset(String key, String code) override {
    widgets.presets->addItem(key.c_str());
  }

  //see https://stackoverflow.com/questions/1956407/how-to-redirect-stderr-in-python-via-python-c-api


  //bindModel
  virtual void bindModel(ScriptingNode* model) override;

private:

#if VISUS_PYTHON
  PyObject* __stdout__ = nullptr;
  PyObject* __stderr__ = nullptr;
#endif

  QTimer     output_timer;

  //refreshGui
  void refreshGui()
  {
    widgets.txtOutput->setText("");
    widgets.txtCode->setText(model->getCode().c_str());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

  //flushOutputs
  void flushOutputs();

  //showEvent
  virtual  void showEvent(QShowEvent*) override;

  //hideEvent
  virtual void hideEvent(QHideEvent*) override;

};

} //namespace Visus

#endif //VISUS_SCRIPTING_NODE_H

