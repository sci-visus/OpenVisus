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

#include <Visus/TransferFunction.h>

namespace Visus {

/////////////////////////////////////////////////////////////////////
SharedPtr<TransferFunction> TransferFunction::fromArray(Array src)
{
  int nfunctions = src.dtype.ncomponents();
  int nsamples = (int)src.dims[0];

  src = ArrayUtils::smartCast(src, DType(src.dtype.ncomponents(), DTypes::FLOAT64));

  auto ret=std::make_shared<TransferFunction>();
  for (int F = 0; F < nfunctions; F++)
  {
    auto component = src.getComponent(F);
    VisusAssert(component.c_size() == sizeof(Float64) * nsamples);
    std::vector<double> values(component.c_ptr<Float64*>(), component.c_ptr<Float64*>() + nsamples);
    ret->addFunction(std::make_shared<SingleTransferFunction>(guessName(F), guessColor(F), values));
  }

  return ret;
}

/////////////////////////////////////////////////////////////////////
SharedPtr<TransferFunction> TransferFunction::fromString(String content)
{
  StringTree in=StringTree::fromString(content);
  if (!in.valid())
  {
    VisusAssert(false);
    return SharedPtr<TransferFunction>();
  }

  auto ret = std::make_shared<TransferFunction>();
  ret->read(in);
  return ret;
}


StringTree DrawLine(int function, int x1, double y1, int x2, double y2) {
  return StringTree("DrawLine").write("function", function).write("x1", x1).write("y1", y1).write("x2", x2).write("y2", y2);
}

StringTree DrawValues(int function, int x1, int x2, std::vector<double> values) {
  return StringTree("DrawValues").write("function", function).write("x1", x1).write("x2", x2).write("values", values);
}

 
  ////////////////////////////////////////////////////////////////////
void TransferFunction::execute(Archive& ar)
{
  if (ar.name == "SetNumberOfSamples")
  {
    int value;
    ar.read("value", value);
    setNumberOfSamples(value);
    return;
  }

  if (ar.name == "SetNumberOfFunctions")
  {
    int value;
    ar.read("value", value);
    setNumberOfFunctions(value);
    return;
  }

  if (ar.name == "SetInputRange")
  {
    Range value;
    ar.read("value", value);
    setInputRange(input_range);
    return;
  }

  if (ar.name == "SetInputNormalizationMode")
  {
    int value = ArrayUtils::UseDTypeRange;
    ar.read("value", value);
    setInputNormalizationMode(value);
    return;
  }

  if (ar.name == "SetOutputDType")
  {
    DType value;
    ar.read("value", value);
    setOutputDType(value);
    return;
  }

  if (ar.name == "SetOutputRange")
  {
    Range value;
    ar.read("value", value);
    setOutputRange(value);
    return;
  }

  if (ar.name == "SetAttenutation")
  {
    double value;
    ar.read("value", value);
    setAttenutation(value);
    return;
  }

  if (ar.name == "ClearFunctions")
  {
    clearFunctions();
    return;
  }

  if (ar.name == "AddFunction")
  {
    auto single = std::make_shared<SingleTransferFunction>();
    single->read(*ar.getFirstChild());
    addFunction(single);
    return;
  }

  if (ar.name == "RemoveFunction")
  {
    int value;
    ar.read("value", value);
    removeFunction(value);
    return;
  }

  if (ar.name == "DrawLine")
  {
    int function, x1, x2; double y1, y2;
    ar.read("function", function);
    ar.read("x1", x1);
    ar.read("y1", y1);
    ar.read("x2", x2);
    ar.read("y2", y2);
    drawLine(function, x1, y1, x2, y2);
    return;
  }

  if (ar.name == "DrawValues")
  {
    int function, x1, x2; std::vector<double> values;
    ar.read("function", function);
    ar.read("x1", x1);
    ar.read("x2", x2);
    ar.read("values", values);
    drawValues(function, x1, x2, values);
    return;
  }


  Model::execute(ar);
}

////////////////////////////////////////////////////////////////////
Range TransferFunction::computeRange(Array src, int C, Aborted aborted) const {

  if (this->input_normalization_mode == ArrayUtils::UseFixedRange)
    return this->input_range;
  else
    return ArrayUtils::computeRange(src, C, input_normalization_mode, aborted);
}


////////////////////////////////////////////////////////////////////
void TransferFunction::addFunction(SharedPtr<SingleTransferFunction> fn)
{
  if (!functions.empty())
    fn->setNumberOfSamples(getNumberOfSamples());

  beginUpdate(
    StringTree("AddFunction").addChild(fn->encode("Function")),
    StringTree("RemoveFunction").write("index",(int)functions.size()));
  {
    this->default_name = "";
    functions.push_back(fn);
  }
  endUpdate();
}



////////////////////////////////////////////////////////////////////
void TransferFunction::removeFunction(int index)
{
  auto fn = functions[index];

  beginUpdate(
    StringTree("RemoveFunction").write("index", index),
    StringTree("AddFunction").addChild(fn->encode("Function")));
  {
    this->default_name = "";
    Utils::remove(functions, fn);
  }
  endUpdate();
}

////////////////////////////////////////////////////////////////////
void TransferFunction::clearFunctions()
{
  if (functions.empty())
    return;

  beginTransaction();
  {
    while (!functions.empty())
      removeFunction(0);
  }
  endTransaction();
}

////////////////////////////////////////////////////////////////////////////////
void TransferFunction::setNumberOfSamples(int new_value)
{
  auto old_value = this->getNumberOfSamples();
  if (new_value == old_value || functions.empty())
    return;

  beginUpdate(
    StringTree("SetNumberOfSamples").write("value", cstring(new_value)),
    Transaction());
  {
    //this will be executed at the end
    topUndo().addChild(StringTree("SetNumberOfSamples").write("value", cstring(old_value)));

    int x1 = 0;
    int x2 = getNumberOfSamples() - 1;

    for (int F = 0; F < getNumberOfFunctions(); F++)
    {
      auto fn = functions[F];
      topUndo().addChild(DrawValues(F, x1, x2, fn->values));
      fn->setNumberOfSamples(new_value);
    }
  }
  endUpdate();
}

////////////////////////////////////////////////////////////////////////////////
void TransferFunction::setNumberOfFunctions(int value)
{
  if (getNumberOfSamples() == value)
    return;

  int nsamples = functions.empty() ? 256 : getNumberOfSamples();

  beginTransaction();
  {
    while (getNumberOfFunctions() > value)
      removeFunction(getNumberOfFunctions()-1);

    while (getNumberOfFunctions() < value)
    {
      auto name = guessName(getNumberOfFunctions());
      auto color = guessColor(getNumberOfFunctions());
      auto values = std::vector<double>(nsamples, 0.0);
      auto fn = std::make_shared<SingleTransferFunction>(name, color, values);
      addFunction(fn);
    }
  }
  endTransaction();
}

////////////////////////////////////////////////////////////////////
void TransferFunction::drawValues(int function, int x1, int x2, std::vector<double> new_values)
{
  int N = (int)new_values.size();
  VisusReleaseAssert(N==(1+x2-x1));
  VisusReleaseAssert(0<=x1 && x1<=x2 && x2<getNumberOfSamples());

  auto fn = functions[function];

  std::vector<double> old_values;
  for (int x = x1; x<=x2; x++)
    old_values.push_back(fn->values[x]);

  //useless call
  if (new_values == old_values)
    return;

  beginUpdate(
    DrawValues(function, x1, x2, new_values),
    DrawValues(function, x1, x2, old_values));
  {
    this->default_name = "";

    for (int x = x1; x <=x2; x++)
      fn->values[x] = new_values[x-x1];
  }
  endUpdate();
}


////////////////////////////////////////////////////////////////////
void TransferFunction::drawLine(int function, int x1, double y1,int x2, double y2)
{
  if (x2 < x1)
  {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }

  x1 = Utils::clamp(x1, 0, getNumberOfSamples() - 1);
  x2 = Utils::clamp(x2, 0, getNumberOfSamples() - 1);

  auto fn = functions[function];

  std::vector<double> new_values; 
  for (int I=0; I < 1 + x2 - x1; I++)
  {
    double alpha  = (x1 == x2) ? 1.0 : I / (double)(x2 - x1);
    new_values.push_back((1 - alpha) * y1 + alpha * y2);
  }

  std::vector<double> old_values;
  for (int I = 0; I < new_values.size(); I++)
    old_values.push_back(fn->values[x1 + I]);

  //useless call
  if (old_values == new_values)
    return;

  beginUpdate(DrawLine(function, x1, y1, x2, y2), DrawValues(function, x1, x2, old_values));
  {
    this->default_name = "";

    for (int I = 0; I < new_values.size(); I++)
      fn->values[x1 + I] = new_values[I];
  }
  endUpdate();
}



/////////////////////////////////////////////////////////////////////
Array TransferFunction::toArray() const
{
  int nfunctions =(int)functions.size();
  if (!nfunctions)
    return Array();

  Array ret;

  int nsamples=getNumberOfSamples();

  std::vector<double> alpha(nfunctions,1.0);
  for (int F=0;F< nfunctions;F++)
  {
    //RGBA palette
    if (attenuation && nfunctions ==4 && F==3)
      alpha[F]=1.0-attenuation;

    //luminance+alpha
    else if (attenuation && nfunctions ==2 && F==1)
      alpha[F]=1.0-attenuation;
  }

  double vs=output_range.delta();
  double vt=output_range.from;

  if (output_dtype==DTypes::UINT8)
  {
    if (!ret.resize(nsamples,DType(nfunctions,DTypes::UINT8),__FILE__,__LINE__))
      return Array();

    for (int F=0;F< nfunctions;F++)
    {
      auto fn=functions[F];
      GetComponentSamples<Uint8> write(ret,F);
      for (int I=0;I<nsamples;I++)
        write[I]=(Uint8)((alpha[F]*fn->values[I])*vs+vt);
    }
  }
  else
  {
    if (!ret.resize(nsamples,DType(nfunctions,DTypes::FLOAT32),__FILE__,__LINE__))
      return Array();

    for (int F=0;F< nfunctions;F++)
    {
      auto fn=functions[F];
      GetComponentSamples<Float32> write(ret,F);
      for (int I=0;I<nsamples;I++)
        write[I]=(Float32)((alpha[F]*fn->values[I])*vs+vt);
    }
  }

  return ret;
}


///////////////////////////////////////////////////////////
SharedPtr<TransferFunction> TransferFunction::importTransferFunction(String url)
{
  /*
  <nsamples> 
  R G B A
  R G B A
  ...
  */

  String content = Utils::loadTextDocument(url);

  std::vector<String> lines = StringUtils::getNonEmptyLines(content);
  if (lines.empty())
  {
    VisusWarning() << "content is empty";
    return SharedPtr<TransferFunction>();
  }

  int nsamples = cint(lines[0]);
  int nfunctions = 4;
  lines.erase(lines.begin());
  if (lines.size() != nsamples)
  {
    VisusWarning() << "content is of incorrect length";
    return SharedPtr<TransferFunction>();
  }

  auto ret=std::make_shared<TransferFunction>();
  ret->output_dtype=DTypes::UINT8;
  ret->output_range=Range(0, 255, 1);

  for (int F = 0; F < nfunctions; F++)
    ret->functions.push_back(std::make_shared<SingleTransferFunction>(guessName(F), guessColor(F), std::vector<double>(nsamples, 0.0)));

  for (int I = 0; I < nsamples; I++)
  {
    std::istringstream istream(lines[I]);

    for (int F = 0; F < nfunctions; F++)
    {
      double value;
      istream >> value;
      value = value / (nsamples - 1.0);
      ret->functions[F]->values[I] = value;
    }
  }

  return ret;
}


///////////////////////////////////////////////////////////
bool TransferFunction::exportTransferFunction(String filename="")
{
  /*
  <nsamples>
  R G B A
  R G B A
  ...
  */

  int nsamples = getNumberOfSamples();

  if (!nsamples) 
    return false;

  std::ostringstream out;
  out<<nsamples<<std::endl;
  for (int I=0;I<nsamples;I++)
  {
    for (auto fn : functions)
    {
      int value = (int)(fn->values[I] * (nsamples - 1));
      out << value << " ";
    }
    out<<std::endl;
  }

  if (!Utils::saveTextDocument(filename,out.str()))
  {
    VisusWarning()<<"file "<<filename<<" could not be opened for writing";
    return false;
  }

  return true;
}


/////////////////////////////////////////////////////////////////////
void TransferFunction::write(Archive& ar) const
{
  int nsamples = getNumberOfSamples();

  ar.write("default_name", default_name);
  ar.write("nsamples", nsamples);
  ar.write("attenuation", attenuation);
  ar.write("input_range", input_range);
  ar.write("input_normalization_mode", input_normalization_mode);
  ar.write("output_dtype", output_dtype);
  ar.write("output_range", output_range);

  if (!isDefault())
  {
    for (auto fn : functions)
      fn->write(*ar.addChild("function"));
  }
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::read(Archive& ar)
{
  this->functions.clear();

  int nsamples;

  ar.read("default_name", default_name);
  ar.read("nsamples", nsamples);
  ar.read("attenuation", attenuation);
  ar.read("input_range", input_range);
  ar.read("input_normalization_mode", input_normalization_mode);
  ar.read("output_dtype", output_dtype);
  ar.read("output_range", output_range);

  if (this->default_name.empty())
  {
    for (auto Function : ar.getChilds("function"))
    {
      auto single = std::make_shared<SingleTransferFunction>();
      single->read(*Function);
      functions.push_back(single);
    }
  }
  else
  {
    this->functions = getDefault(default_name, nsamples)->functions;
  }
}

} //namespace Visus

