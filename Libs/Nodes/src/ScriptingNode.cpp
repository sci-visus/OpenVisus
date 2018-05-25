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
#include <Visus/VisusConfig.h>
#include <Visus/PythonEngine.h>


namespace Visus {


/////////////////////////////////////////////////////////////////////////////////////
class ScriptingNode::MyJob : public NodeJob
{
public:

  ScriptingNode*             node;
  SharedPtr<Array>           input;
  SharedPtr<ReturnReceipt>   return_receipt;
  bool                       bDataOutputPortConnected;
  Int64                      incremental_last_publish_time = -1;
  int                        max_publish_msec; //Only here until it can be passed into node 
  String                     code;
  SharedPtr<PythonEngine> engine;

  //constructor
  MyJob(ScriptingNode* node_, SharedPtr<Array> input_,SharedPtr<ReturnReceipt> return_receipt_)
    : node(node_), input(input_),return_receipt(return_receipt_)
  {
    this->engine = node->engine;
    this->bDataOutputPortConnected=node->isOutputConnected("data");
    this->max_publish_msec=cint(VisusConfig::readString("Configuration/ScriptingNode/max_dopublish_msec","600"));
    this->code = node->getCode();
  }

  //valid
  bool valid() {
    return node && input && bDataOutputPortConnected;
  }

  //runJob
  virtual void runJob() override
  {
    if (!valid() || aborted()) 
      return;

    if (aborted())
      return;

    if (code.empty())
    {
      doPublish(input,/*bIncremental*/false);
      return;
    }

    SharedPtr<Array> output;
    try {
      output = scriptingProcessInput(code, *input, aborted);
    }
    catch (Exception ex)
    {
      String error_msg=StringUtils::format()<<"ERROR:\n"
        << "FILE "<<ex.getFile()<<"\n"
        << "LINE "<<ex.getLine() << "\n"
        << ex.getExpression() << "\n";
      printMessage(error_msg);
      return;
    }

    if (!output) 
    {
      printMessage("ERROR output is not an Array");
      return;
    }

    doPublish(output);
  }

  //printMessage
  void printMessage(String msg)
  {
    if (msg.empty())
      return;

    for (auto it : node->views)
    {
      if (auto view = dynamic_cast<ScriptingNodeBaseView*>(it))
        view->appendOutput(msg);
    }
  }

  //scriptingProcessInput
  SharedPtr<Array> scriptingProcessInput(String code, Array input, Aborted aborted)
  {
    ScopedAcquireGil acquire_gil;
    
    Array output;
    {
      DoAtExit do_at_exit([this]() {

        engine->delModuleAttr("input");
        engine->delModuleAttr("aborted");
        engine->delModuleAttr("output");
        engine->delModuleAttr("doPublish");
      });

      engine->setModuleAttr("input"  , input);
      engine->setModuleAttr("aborted", aborted);

      engine->addModuleFunction("doPublish", [this](PyObject *self, PyObject *args)
      {
        auto output = engine->getModuleArrayAttr("output");
        if (output)
          doPublish(std::make_shared<Array>(output),/*bIncremental*/true);
        return (PyObject*)nullptr;
      });

      engine->execCode(code);

      output = engine->getModuleArrayAttr("output");
    }

    return std::make_shared<Array>(output);
  };

  //doPublish
  void doPublish(SharedPtr<Array> output,bool bIncremental=false)
  {
    if (aborted() || !output)
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

    output->shareProperties(*input);
    
    //a projection happened?
    int pdim = input->dims.getPointDim();
    for (int D = 0; D < pdim; D++)
    {
      if (output->dims[D] == 1 && input->dims[D] > 1)
      {
        auto box = output->bounds.getBox();
        box.p2[D] = box.p1[D];
        output->bounds = Position(output->bounds.getTransformation(), box);
        output->clipping = Position::invalid(); //disable clipping
      }
    }

    if (!bIncremental)
      printMessage(StringUtils::format() << "Array " << output->dims.toString());

    auto msg=std::make_shared<DataflowMessage>();

    //only if final I eventually release the pass thought return receipt
    if (!bIncremental && return_receipt)
      msg->setReturnReceipt(return_receipt);

    msg->writeContent("data",output);
    node->publish(msg);
  }

};

///////////////////////////////////////////////////////////////////////
ScriptingNode::ScriptingNode(String name)  : Node(name)
{
  engine = std::make_shared<PythonEngine>(false);

  addInputPort("data");
  addOutputPort("data");

  //redirectOutputTo
  engine->redirectOutputTo([this](String msg) {
    for (auto it : views) {
      if (auto view = dynamic_cast<ScriptingNodeBaseView*>(it))
        view->appendOutput(msg);
    }
  });
}

///////////////////////////////////////////////////////////////////////
ScriptingNode::~ScriptingNode()
{
}

///////////////////////////////////////////////////////////////////////
bool ScriptingNode::processInput()
{
  abortProcessing();

  //I'm using the shared engine...
  joinProcessing(); 

  // important to do before readInput
  auto return_receipt=ReturnReceipt::createPassThroughtReceipt(this);
  auto input = readInput<Array>("data");
  if (!input)
    return  false;

  guessPresets(input);

  auto process_job=std::make_shared<MyJob>(this, input,return_receipt);
  if (!process_job->valid()) 
    return false;
  
  this->bounds= input->bounds;

  addNodeJob(process_job);
  return true;
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
void ScriptingNode::guessPresets(SharedPtr<Array> input)
{
  VisusAssert(VisusHasMessageLock());

  if (!input)
    return;

  if (last_dtype == input->dtype)
    return;

  last_dtype = input->dtype;

  clearPresets();

  addPreset("Identity", "output=input");

  int N = input->dtype.ncomponents();
  {
    std::vector<String> inputs;
    for (int I = 0; I<N; I++)
      inputs.push_back(StringUtils::format() << "input[" << I << "]");

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
  if (input->dtype.ncomponents()>=3)
  {
    addPreset("Grayscale (Photometric ITU-R)", 
      "R,G,B=(0.2126*input[0] , 0.7152*input[1] , 0.0722*input[2])\n" 
      "output=R+G+B\n");

    addPreset("Grayscale (Digital CCIR601)", 
      "R,G,B=(0.2990*input[0] , 0.5870*input[1] , 0.1140*input[2])\n" 
      "output=R+G+B\n");
  }

  //filters
  {
    addPreset("Filters/brightnessContrast",
      "brightness,contrast=(0.0,1.0)\noutput=ArrayUtils.brightnessContrast(input,brightness,contrast,aborted)\n");

    addPreset("Filters/threshold",
      "level=0.5\noutput=ArrayUtils.threshold(input,level,aborted)\n");

    addPreset("Filters/invert",
      "output=ArrayUtils.invert(input,aborted)\n");

    addPreset("Filters/levels",
      "gamma, in_min, in_max, out_min, out_max=(1.0, 0.0, 1.0, 0.0, 1.0)\noutput=ArrayUtils.levels(input, gamma, in_min, in_max, out_min, out_max, aborted)\n");

    addPreset("Filters/hueSaturationBrightness",
      "hue,saturation,brightness=(0.0, 0.0, 0.0)\noutput=ArrayUtils.hueSaturationBrightness(input,hue,saturation,brightness,aborted)\n");
  }

  auto createConvolutionPreset = [](String kernel) {
    return (StringUtils::format() 
      << "kernel=" << kernel << "\n"
      << "output=ArrayUtils.convolve(input,Array.fromPyArray(kernel),aborted)\n").str();
  };

  //see http://www.mif.vu.lt/atpazinimas/dip/FIP/fip-Derivati.html
  //see http://www.johnloomis.org/ece563/notes/filter/second/second.html
  addPreset("2d/Blur",          createConvolutionPreset("[[1/9.0,1/9.0,1/9.0], [1/9.0,1/9.0,1/9.0] , [1/9.0,1/9.0,1/9.0]]"));
  addPreset("2d/Mean removal",  createConvolutionPreset("[[-1.0,-1.0,-1.0], [-1.0, 9.0,-1.0] ,[-1.0,-1.0,-1.0]]"));
  addPreset("2d/Sharpen",       createConvolutionPreset("[[ 0.0,-2.0, 0.0], [-2.0,11.0,-2.0], [ 0.0,-2.0, 0.0]]"));
  addPreset("2d/Emboss",        createConvolutionPreset("[[ 2.0, 0.0, 0.0], [ 0.0,-1.0, 0.0], [ 0.0, 0.0,-1.0]]"));
  addPreset("2d/Emboss Subtle", createConvolutionPreset("[[ 1.0, 1.0,-1.0], [ 1.0, 3.0,-1.0], [ 1.0,-1.0,-1.0]]"));
  addPreset("2d/Edge Detect",   createConvolutionPreset("[[ 1.0, 1.0, 1.0], [ 1.0,-7.0, 1.0], [ 1.0, 1.0, 1.0]]"));
  
  addPreset("2d/First Derivative/Backward", createConvolutionPreset("[ -1.0,  1.0, 0.0 ]"));
  addPreset("2d/First Derivative/Central",  createConvolutionPreset("[ -1.0,  0.0, 1.0 ]"));
  addPreset("2d/First Derivative/Forward",  createConvolutionPreset("[  0.0, -1.0, 1.0 ]"));
  addPreset("2d/First Derivative/Prewitt",  createConvolutionPreset("[[ 1.0,0.0,-1.0], [ 1.0,0.0,-1.0], [ 1.0,0.0,-1.0]]"));
  addPreset("2d/First Derivative/Sobel",    createConvolutionPreset("[[-1.0,0.0, 1.0], [-2.0,0.0, 2.0], [-1.0,0.0, 1.0]]"));

  addPreset("2d/Second Derivative/Basic",                        createConvolutionPreset("[1.0,-2.0,1.0]"));
  addPreset("2d/Second Derivative/Basic with smoothing (i)",     createConvolutionPreset("[[1.0,-2.0,1.0], [2.0,-4.0,2.0], [1.0,-2.0,1.0]]"));
  addPreset("2d/Second Derivative/Basic with smoothing (ii)",    createConvolutionPreset("[[1.0,-2.0,1.0], [1.0,-2.0,1.0], [1.0,-2.0,1.0]]"));
  addPreset("2d/Second Derivative/Laplacian",                    createConvolutionPreset("[[0.0, 1.0,0.0], [1.0,-4.0,1.0], [0.0, 1.0,0.0]]"));
  addPreset("2d/Second Derivative/Laplacian with smoothing (i)", createConvolutionPreset("[[1.0, 0.0,1.0], [0.0,-4.0,0.0], [1.0, 0.0,1.0]]"));

  addPreset("3d/Sobel Derivative",
    "k1=[\n"
    "  [[ 1.0,  2.0,  1.0], [ 2.0,  4.0,  2.0],  [ 1.0,  2.0,  1.0]],\n"
    "  [[ 0.0,  0.0,  0.0], [ 0.0,  0.0,  0.0],  [ 0.0,  0.0,  0.0]],\n"
    "  [[-1.0, -2.0, -1.0], [-2.0, -4.0, -2.0],  [-1.0, -2.0 ,-1.0]]]\n"
    "\n"
    "k2=[\n"
    " [[1.0,  0.0,  -1.0],  [2.0,  0.0,  -2.0],  [1.0,  0.0, -1.0]],\n"
    " [[2.0,  0.0,  -2.0],  [4.0,  0.0,  -4.0],  [2.0,  0.0, -2.0]],\n"
    " [[1.0,  0.0,  -1.0],  [2.0,  0.0,  -2.0],  [1.0,  0.0 ,-1.0]]]\n"
    "\n"
    "k3=[\n"
    " [[1.0,  2.0,  1.0],  [0.0,  0.0,  0.0], [-1.0, -2.0, -1.0]],\n"
    " [[2.0,  4.0,  2.0],  [0.0,  0.0,  0.0], [-2.0, -4.0, -2.0]],\n"
    " [[1.0,  2.0,  1.0],  [0.0,  0.0,  0.0], [-1.0, -2.0 ,-1.0]]]\n"
    "\n"
    "o1=ArrayUtils.convolve(input,Array.fromPyArray(k1))\n"
    "o2=ArrayUtils.convolve(input,Array.fromPyArray(k2))\n"
    "o3=ArrayUtils.convolve(input,Array.fromPyArray(k3))\n"
    "\n"
    "output=o1*o1+o2*o2+o3*o3\n");

  //broken right now
  addPreset("3d/MedianHybrid",
    "k=[1.0]\n"
    "output=ArrayUtils.medianHybrid(input,Array.fromPyArray(k))\n");

  //broken right now?
  addPreset("3d/Median",
    "k=[1.0]\n"
    "percent=50\n"
    "output=ArrayUtils.median(input,Array.fromPyArray(k),percent)\n");

}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::writeToObjectStream(ObjectStream& ostream)
{
  Node::writeToObjectStream(ostream);
  if (!code.empty())
  {
    ostream.pushContext("code");
    ostream.writeText(code);
    ostream.popContext("code");
  }
}

///////////////////////////////////////////////////////////////////////
void ScriptingNode::readFromObjectStream(ObjectStream& istream)
{
  Node::readFromObjectStream(istream);

  if (istream.pushContext("code"))
  {
    setCode(istream.readText());
    istream.popContext("code");
  }
}

} //namespace Visus


