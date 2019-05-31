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

#include <Visus/ScriptingNodeView.h>


namespace Visus {

static CriticalSection     outputs_lock;
static std::vector<String> outputs;


////////////////////////////////////////////////////////////////
#if VISUS_PYTHON
static PyObject* WriteMethod(PyObject* self, PyObject* args)
{
  if (!PyTuple_Check(args))
    return NULL;

  {
    ScopedLock lock(outputs_lock);
    for (int I = 0, N = (int)PyTuple_Size(args); I < N; I++) {
      auto obj = PyTuple_GetItem(args, I);
      auto s = PythonEngine::convertToString(obj);
      outputs.push_back(s);
    }
  }

  auto ret = Py_None; 
  Py_INCREF(ret); 
  return ret;
}

////////////////////////////////////////////////////////////////
static PyObject* FlushMethod(PyObject* self, PyObject* args)
{
  auto ret = Py_None; 
  Py_INCREF(ret); 
  return ret;
}


static  PyMethodDef __methods__[3] =
{
  { "write", WriteMethod, METH_VARARGS, "doc for write" },
  { "flush", FlushMethod, METH_VARARGS, "doc for flush" },
  { 0, 0, 0, 0 } // sentinel
};

#if PY_MAJOR_VERSION >= 3
static PyModuleDef RedirectOutputModule =
{
  PyModuleDef_HEAD_INIT,
  "RedirectOutputModule",
  "doc for RedirectOutputModule",
  -1,
  __methods__,
};
#endif

#endif //
////////////////////////////////////////////////////////////////////////
ScriptingNodeView::ScriptingNodeView(ScriptingNode* model)
{
  VisusAssert(VisusHasMessageLock());

  connect(&output_timer, &QTimer::timeout, [this]() {
    flushOutputs();
  });

  output_timer.start(200);

  if (model)
    bindModel(model);
}

////////////////////////////////////////////////////////////////////////
ScriptingNodeView::~ScriptingNodeView() {
  bindModel(nullptr);
}


////////////////////////////////////////////////////////////////////////
void ScriptingNodeView::showEvent(QShowEvent *)
{
#if VISUS_PYTHON

  ScopedAcquireGil acquire_gil;
  this->__stdout__ = PySys_GetObject((char*)"stdout");
  this->__stderr__ = PySys_GetObject((char*)"stderr");
#if PY_MAJOR_VERSION >= 3
  auto redirect_module = PyModule_Create(&RedirectOutputModule);
#else
  auto redirect_module = Py_InitModule("RedirectOutputModule", __methods__);
#endif

  PySys_SetObject((char*)"stdout", redirect_module);
  PySys_SetObject((char*)"stderr", redirect_module);

#endif 
}

////////////////////////////////////////////////////////////////////////
void ScriptingNodeView::hideEvent(QHideEvent *)
{
#if VISUS_PYTHON
  ScopedAcquireGil acquire_gil;
  PySys_SetObject((char*)"stdout", this->__stdout__);
  PySys_SetObject((char*)"stderr", this->__stderr__);
#endif
}


////////////////////////////////////////////////////////////////////////
void ScriptingNodeView::bindModel(ScriptingNode* model) 
{
  if (this->model)
  {
    QUtils::clearQWidget(this);
    widgets = Widgets();
  }

  ScriptingNodeBaseView::bindModel(model);

  if (this->model)
  {
    QVBoxLayout* layout = new QVBoxLayout();

    layout->addWidget(new QLabel("Input"));

    //presets
    auto presets = model->getPresets();

    layout->addWidget(widgets.presets = GuiFactory::CreateComboBox(presets.empty() ? "" : presets[0], presets, [this](const String value) {
      auto code = this->model->getPresetCode(value);
      this->model->setCode(code);
    }));

    //code
    layout->addWidget(widgets.txtCode = GuiFactory::CreateTextEdit());

    {
      auto row = (new QHBoxLayout());;

      row->addWidget(widgets.btnRun = GuiFactory::CreateButton("Run", [this](bool) {
        this->model->setCode(cstring(widgets.txtCode->toPlainText()));
      }));

      layout->addLayout(row);
    }

    layout->addWidget(new QLabel("Output"));

    //output
    layout->addWidget(widgets.txtOutput = GuiFactory::CreateTextEdit());

    setLayout(layout);
    refreshGui();
  }
}


////////////////////////////////////////////////////////////////////////
void ScriptingNodeView::flushOutputs()
{
  VisusAssert(VisusHasMessageLock());

  ScopedLock lock(outputs_lock);
  for (auto output : outputs)
  {
#if 1
    widgets.txtOutput->moveCursor(QTextCursor::End);
    widgets.txtOutput->insertPlainText(output.c_str());
    widgets.txtOutput->insertPlainText("\n");
    widgets.txtOutput->moveCursor(QTextCursor::End);
#else
    widgets.txtOutput->setTextColor(QUtils::convert<QColor>(Colors::Red));
    widgets.txtOutput->append((StringUtils::format() << "[" << Time::now().getFormattedLocalTime() << "]").str().c_str());
    widgets.txtOutput->setTextColor(QUtils::convert<QColor>(Colors::Black));
    widgets.txtOutput->append((StringUtils::format() << output).str().c_str());
#endif
  }

  outputs.clear();
}

} //namespace Visus


