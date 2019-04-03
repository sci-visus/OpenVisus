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

#ifndef VISUS_SCRIPTING_NODE_VIEW_H
#define VISUS_SCRIPTING_NODE_VIEW_H

#include <Visus/AppKit.h>
#include <Visus/ScriptingNode.h>
#include <Visus/Model.h>
#include <Visus/GuiFactory.h>

#include <QTimer>

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API ScriptingNodeView : 
  public QFrame, 
  public ScriptingNodeBaseView
{
public:

  VISUS_NON_COPYABLE_CLASS(ScriptingNodeView)

  class VISUS_APPKIT_API Widgets
  {
  public:
    QTextEdit*          txtCode = nullptr;
    QTextEdit*          txtOutput = nullptr;
    QToolButton*        btnRun = nullptr;
    QComboBox*          presets = nullptr;
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

  PyObject* __stdout__ = nullptr;
  PyObject* __stderr__ = nullptr;

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
  virtual  void showEvent(QShowEvent *) override;

  //hideEvent
  virtual void hideEvent(QHideEvent *) override;

};


} //namespace Visus

#endif //VISUS_SCRIPTING_NODE_VIEW_H

