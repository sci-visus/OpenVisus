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

#include <Visus/ScriptingNode.h>
#include <Visus/StringTree.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

namespace Visus {


/////////////////////////////////////////////////////////////////////////////////////
#if VISUS_PYTHON
class ScriptingNode::MyJob : public NodeJob
{
public:

  ScriptingNode*             node;
  Array                      input;
  SharedPtr<ReturnReceipt>   return_receipt;
  bool                       bDataOutputPortConnected;
  Int64                      incremental_last_publish_time = -1;
  int                        max_publish_msec; //Only here until it can be passed into node 
  String                     code;
  SharedPtr<PythonEngine>    engine;

  //constructor
  MyJob(ScriptingNode* node_, Array input_,SharedPtr<ReturnReceipt> return_receipt_)
    : node(node_), input(input_),return_receipt(return_receipt_)
  {
    this->engine = node->engine;
    this->bDataOutputPortConnected=node->isOutputConnected("array");
    this->max_publish_msec=node->max_publish_msec;
    this->code = node->getCode();
  }

  //valid
  bool valid() {
    return node && input && bDataOutputPortConnected;
  }

  //doPublish
  void doPublish(Array output, bool bIncremental)
  {
    if (aborted())
      return;

    if (bIncremental)
    {
      Time current_time = Time::now();

      //postpost a little?
      if (incremental_last_publish_time > 0)
      {
        auto enlapsed_msec = current_time.getUTCMilliseconds() - incremental_last_publish_time;
        if (enlapsed_msec < this->max_publish_msec)
          return;
      }

      incremental_last_publish_time = current_time.getUTCMilliseconds();
    }

    output.shareProperties(input);

    //a projection happened?
#if 1
    if (input.dims != output.dims)
    {
      //disable clipping
      output.clipping = Position::invalid(); 

      //fix bounds
      auto T   = output.bounds.getTransformation();
      auto box = output.bounds.getBoxNd();
      for (int D = 0; D < input.dims.getPointDim(); D++)
      {
        if (input.dims[D] > 1 && output.dims[D] == 1)
          box.p2[D] = box.p1[D];
      }
      output.bounds = Position(T, box);
    }
#endif

    if (!bIncremental)
    {
      ScopedAcquireGil acquire_gil;
      engine->printMessage(cstring("Array",output.dims,"\n"));
    }

    DataflowMessage msg;

    //only if final I eventually release the pass thought return receipt
    if (!bIncremental && return_receipt)
      msg.setReturnReceipt(return_receipt);

    msg.writeValue("array", output);
    node->publish(msg);
  }

  //clearModuleAttrs
  void clearModuleAttrs(ScopedAcquireGil&) {
    engine->delModuleAttr("input");
    engine->delModuleAttr("aborted");
    engine->delModuleAttr("output");
    engine->delModuleAttr("doPublish");
  }

  //runJob
  virtual void runJob() override
  {
    if (!valid() || aborted()) 
      return;

    //pass throught
    if (code.empty())
    {
      doPublish(input,false);
      return;
    }

    Array output;
    try 
    {
      auto t1 = Time::now();
      PrintInfo("Executing script...");
      ScopedAcquireGil acquire_gil;
      engine->setModuleAttr("input", input);
      engine->setModuleAttr("aborted", aborted);
      engine->addModuleFunction("doPublish", [this](PyObject *self, PyObject *args)
      {
        auto output = engine->getModuleArrayAttr("output");
        doPublish(output,true);
        return (PyObject*)nullptr;
      });

      engine->execCode(code);
      output = engine->getModuleArrayAttr("output");
      clearModuleAttrs(acquire_gil);
      PrintInfo("Executed script in", t1.elapsedMsec(), "msec");
    }
    catch (std::exception ex)
    {
      ScopedAcquireGil acquire_gil;
      engine->printMessage(ex.what());
      clearModuleAttrs(acquire_gil);
      return;
    }

    doPublish(output,false);
  }

};
#endif //VISUS_PYTHON

///////////////////////////////////////////////////////////////////////
ScriptingNode::ScriptingNode()  
{
#if VISUS_PYTHON
  this->engine = std::make_shared<PythonEngine>(false);
#endif

  addInputPort("array");
  addOutputPort("array");
}

///////////////////////////////////////////////////////////////////////
ScriptingNode::~ScriptingNode()
{
}


//////////////////////////////////////////////////////////////////////////
void ScriptingNode::execute(Archive& ar)
{
  if (ar.name == "SetMaxPublishMSec") {
    int value;
    ar.read("value", value);
    setMaxPublishMSec(value);
    return;
  }

  if (ar.name == "SetCode") {
    String value;
    ar.read("value", value);
    setCode(value);
    return;
  }

  return Node::execute(ar);
}

///////////////////////////////////////////////////////////////////////
bool ScriptingNode::processInput()
{
  abortProcessing();

  //I'm using the shared engine...
  joinProcessing(); 

  // important to do before readValue
  auto return_receipt=createPassThroughtReceipt();
  auto input = readValue<Array>("array");
  if (!input)
    return  false;

  guessPresets(*input);

  this->bounds = input->bounds;

  //pass throught
#if VISUS_PYTHON

  auto process_job = std::make_shared<MyJob>(this, *input, return_receipt);
  if (!process_job->valid())
    return false;
  addNodeJob(process_job);

#else

  DataflowMessage msg;
  msg.setReturnReceipt(return_receipt);
  msg.writeValue("array", input);
  this->publish(msg);

#endif

  return true;
}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::addUserInput(String key, Array value) 
{
#if VISUS_PYTHON
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
#else
  ThrowException(" not supported");
#endif
}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::clearPresets()
{
  presets.keys.clear();
  presets.code.clear();

  for (auto it : views) {
    if (auto view = dynamic_cast<ScriptingNodeBaseView*>(it))
      view->clearPresets();
  }
}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::addPreset(String key, String code)
{
  VisusAssert(Utils::find(presets.keys,key) == -1);
  presets.keys.push_back(key);
  presets.code.push_back(code);

  for (auto it : views) {
    if (auto view = dynamic_cast<ScriptingNodeBaseView*>(it))
      view->addPreset(key, code);
  }
}


///////////////////////////////////////////////////////////////
void ScriptingNode::guessPresets(Array input)
{
  VisusAssert(VisusHasMessageLock());

  if (!input)
    return;

  if (last_dtype == input.dtype)
    return;

  last_dtype = input.dtype;

  clearPresets();

  addPreset("Identity", "output=input");

  int N = input.dtype.ncomponents();
  {
    std::vector<String> inputs;
    for (int I = 0; I<N; I++)
      inputs.push_back(concatenate("input[",I,"]"));

    for (int I = 0; I<N; I++)
      addPreset(inputs[I], "output="+inputs[I]);

    for (int I = 0; I<N; I++)
      addPreset(inputs[I]+" (floa64)", "output=ArrayUtils.cast(" + inputs[I]+",DType.fromString('float64'),aborted)");

    addPreset("Module" , "output=ArrayUtils.module(input,aborted)");
    addPreset("Module2", "output=ArrayUtils.module2(input,aborted)");

    auto components = StringUtils::join(inputs, ",", "(", ")");

    addPreset("Add"              , "components=" + components + "\noutput=ArrayUtils.add(components,aborted)");
    addPreset("Sub"              , "components=" + components + "\noutput=ArrayUtils.sub(components,aborted)");
    addPreset("Mul"              , "components=" + components + "\noutput=ArrayUtils.mul(components,aborted)");
    addPreset("Div"              , "components=" + components + "\noutput=ArrayUtils.div(components,aborted)");
    addPreset("Min"              , "components=" + components + "\noutput=ArrayUtils.min(components,aborted)");
    addPreset("Max"              , "components=" + components + "\noutput=ArrayUtils.max(components,aborted)");
    addPreset("Average"          , "components=" + components + "\noutput=ArrayUtils.average(components,aborted)");
    addPreset("StandardDeviation", "components=" + components + "\noutput=ArrayUtils.standardDeviation(components,aborted)");
    addPreset("Median"           , "components=" + components + "\noutput=ArrayUtils.median(components,aborted)");
  }

  //see http://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
  if (input.dtype.ncomponents()>=3)
  {
    addPreset("Grayscale (Photometric ITU-R)", StringUtils::joinLines({
      "R,G,B=(0.2126*input[0] , 0.7152*input[1] , 0.0722*input[2])",
      "output=R+G+B"
    }));

    addPreset("Grayscale (Digital CCIR601)", StringUtils::joinLines({
      "R,G,B=(0.2990*input[0] , 0.5870*input[1] , 0.1140*input[2])",
      "output=R+G+B"
    }));
  }

  //filters
  {
    addPreset("Filters/brightnessContrast", StringUtils::joinLines({
      "brightness,contrast=(0.0,1.0)",
      "output=ArrayUtils.brightnessContrast(input,brightness,contrast,aborted)",
    }));

    addPreset("Filters/threshold", StringUtils::joinLines({
      "level=0.5",
      "output=ArrayUtils.threshold(input,level,aborted)"
    }));

    addPreset("Filters/invert",
      "output=ArrayUtils.invert(input,aborted)");

    addPreset("Filters/levels", StringUtils::joinLines({
      "gamma, in_min, in_max, out_min, out_max=(1.0, 0.0, 1.0, 0.0, 1.0)",
      "output=ArrayUtils.levels(input, gamma, in_min, in_max, out_min, out_max, aborted)"
    }));

    addPreset("Filters/hueSaturationBrightness", StringUtils::joinLines({
      "hue,saturation,brightness=(0.0, 0.0, 0.0)",
      "output=ArrayUtils.hueSaturationBrightness(input,hue,saturation,brightness,aborted)"
    }));
  }

  auto addConvolutionPreset = [&](String name, String kernel) {
    addPreset(name,StringUtils::joinLines({
      "import numpy",
      "kernel=Array.fromNumPy(numpy.array(" + kernel + ",dtype=numpy.float64))",
      "output=ArrayUtils.convolve(input,kernel,aborted)",
    }));
  };

  //see http://www.mif.vu.lt/atpazinimas/dip/FIP/fip-Derivati.html
  //see http://www.johnloomis.org/ece563/notes/filter/second/second.html
  addConvolutionPreset("2d/Blur",          "[[1/9.0,1/9.0,1/9.0], [1/9.0,1/9.0,1/9.0] , [1/9.0,1/9.0,1/9.0]]");
  addConvolutionPreset("2d/Mean removal",  "[[-1.0,-1.0,-1.0], [-1.0, 9.0,-1.0] ,[-1.0,-1.0,-1.0]]");
  addConvolutionPreset("2d/Sharpen",       "[[ 0.0,-2.0, 0.0], [-2.0,11.0,-2.0], [ 0.0,-2.0, 0.0]]");
  addConvolutionPreset("2d/Emboss",        "[[ 2.0, 0.0, 0.0], [ 0.0,-1.0, 0.0], [ 0.0, 0.0,-1.0]]");
  addConvolutionPreset("2d/Emboss Subtle", "[[ 1.0, 1.0,-1.0], [ 1.0, 3.0,-1.0], [ 1.0,-1.0,-1.0]]");
  addConvolutionPreset("2d/Edge Detect",   "[[ 1.0, 1.0, 1.0], [ 1.0,-7.0, 1.0], [ 1.0, 1.0, 1.0]]");
  
  addConvolutionPreset("2d/First Derivative/Backward", "[ -1.0,  1.0, 0.0 ]");
  addConvolutionPreset("2d/First Derivative/Central",  "[ -1.0,  0.0, 1.0 ]");
  addConvolutionPreset("2d/First Derivative/Forward",  "[  0.0, -1.0, 1.0 ]");
  addConvolutionPreset("2d/First Derivative/Prewitt",  "[[ 1.0,0.0,-1.0], [ 1.0,0.0,-1.0], [ 1.0,0.0,-1.0]]");
  addConvolutionPreset("2d/First Derivative/Sobel",    "[[-1.0,0.0, 1.0], [-2.0,0.0, 2.0], [-1.0,0.0, 1.0]]");

  addConvolutionPreset("2d/Second Derivative/Basic",                        "[1.0,-2.0,1.0]");
  addConvolutionPreset("2d/Second Derivative/Basic with smoothing (i)",     "[[1.0,-2.0,1.0], [2.0,-4.0,2.0], [1.0,-2.0,1.0]]");
  addConvolutionPreset("2d/Second Derivative/Basic with smoothing (ii)",    "[[1.0,-2.0,1.0], [1.0,-2.0,1.0], [1.0,-2.0,1.0]]");
  addConvolutionPreset("2d/Second Derivative/Laplacian",                    "[[0.0, 1.0,0.0], [1.0,-4.0,1.0], [0.0, 1.0,0.0]]");
  addConvolutionPreset("2d/Second Derivative/Laplacian with smoothing (i)", "[[1.0, 0.0,1.0], [0.0,-4.0,0.0], [1.0, 0.0,1.0]]");

  addPreset("3d/Sobel Derivative",
    StringUtils::joinLines({
      "import numpy",
      "k1=Array.fromNumPy(numpy.array([",
      "  [[ 1.0,  2.0,  1.0], [ 2.0,  4.0,  2.0],  [ 1.0,  2.0,  1.0]],",
      "  [[ 0.0,  0.0,  0.0], [ 0.0,  0.0,  0.0],  [ 0.0,  0.0,  0.0]],",
      "  [[-1.0, -2.0, -1.0], [-2.0, -4.0, -2.0],  [-1.0, -2.0 ,-1.0]]], dtype=numpy.float64))",
      "k2=Array.fromNumPy(numpy.array([",
      " [[1.0,  0.0,  -1.0],  [2.0,  0.0,  -2.0],  [1.0,  0.0, -1.0]],",
      " [[2.0,  0.0,  -2.0],  [4.0,  0.0,  -4.0],  [2.0,  0.0, -2.0]],",
      " [[1.0,  0.0,  -1.0],  [2.0,  0.0,  -2.0],  [1.0,  0.0 ,-1.0]]], dtype=numpy.float64))",
      "k3=Array.fromNumPy(numpy.array([",
      " [[1.0,  2.0,  1.0],  [0.0,  0.0,  0.0], [-1.0, -2.0, -1.0]],",
      " [[2.0,  4.0,  2.0],  [0.0,  0.0,  0.0], [-2.0, -4.0, -2.0]],",
      " [[1.0,  2.0,  1.0],  [0.0,  0.0,  0.0], [-1.0, -2.0 ,-1.0]]], dtype=numpy.float64))",
      "o1=ArrayUtils.convolve(input,k1)",
      "o2=ArrayUtils.convolve(input,k2)",
      "o3=ArrayUtils.convolve(input,k3)",
      "",
      "output=o1*o1+o2*o2+o3*o3"
  }));

  //broken right now
  addPreset("3d/MedianHybrid",StringUtils::joinLines({
    "import numpy",
    "kernel=Array.fromNumPy(numpy.array([1.0], dtype=numpy.float64)",
    "output=ArrayUtils.medianHybrid(input,kernel)"
  }));

  //broken right now?
  addPreset("3d/Median", StringUtils::joinLines({
    "import numpy",
    "kernel=Array.fromNumPy(numpy.array([1.0], dtype=numpy.float64))",
    "percent=50",
    "output=ArrayUtils.median(input,kernel,percent)"
  }));
}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::write(Archive& ar) const
{
  Node::write(ar);
  ar.write("max_publish_msec", max_publish_msec);
  ar.writeText("code", code);
}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::read(Archive& ar)
{
  Node::read(ar);
  ar.read("max_publish_msec", max_publish_msec);
  ar.readText("code", code);
}


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
void ScriptingNodeView::showEvent(QShowEvent*)
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
void ScriptingNodeView::hideEvent(QHideEvent*)
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
    //widgets.txtOutput->insertPlainText("\n");
    widgets.txtOutput->moveCursor(QTextCursor::End);
#else
    widgets.txtOutput->setTextColor(QUtils::convert<QColor>(Colors::Red));
    widgets.txtOutput->append(cstring("[", Time::now().getFormattedLocalTime(), "]").c_str());
    widgets.txtOutput->setTextColor(QUtils::convert<QColor>(Colors::Black));
    widgets.txtOutput->append(output.c_str());
#endif
  }

  outputs.clear();
}

} //namespace Visus

