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
  ret->readFrom(in);
  return ret;
}

  ////////////////////////////////////////////////////////////////////
void TransferFunction::executeAction(StringTree in)
{
  if (in.name == "Assign")
  {
    readFrom(in);

    TransferFunction other;
    other.readFrom(in);
    (*this) = other;
    return;
  }

  if (in.name == "ClearFunctions")
  {
    clearFunctions();
    return;
  }


  if (in.name == "AddFunction")
  {
    auto single = std::make_shared<SingleTransferFunction>();
    single->readFrom(in);
    addFunction(single);
    return;
  }

  if (in.name == "RemoveFunction")
  {
    int index = in.readInt("index");
    removeFunction(index);
    return;
  }

  if (in.name == "DrawLine")
  {
    auto p1 = Point2d::fromString(in.read("p1"));
    auto p2 = Point2d::fromString(in.read("p2"));
    std::vector<int> selected;
    for (auto it : StringUtils::split(in.read("selected")))
      selected.push_back(cint(it));
    drawLine(p1, p2, selected);
    return;
  }

  if (in.name == "SetProperty")
  {
    auto name = in.readString("name");


    if (name == "num_samples")
    {
      auto value = in.readInt("value");
      setNumberOfSamples(value);
      return;
    }

    if (in.name == "num_functions")
    {
      auto value = in.readInt("value");
      setNumberOfFunctions(value);
      return;
    }

    if (name == "input_range")
    {
      ComputeRange compute_range;
      compute_range.mode = (ComputeRange::Mode)in.readInt("mode");
      compute_range.custom_range = Range::fromString(in.readString("custom_range"));
      setInputRange(compute_range);
      return;
    }

    if (name == "output_dtype")
    {
      auto value = DType::fromString(in.readString("value"));
      setOutputDType(value);
      return;
    }

    if (name == "output_range")
    {
      auto value = Range::fromString(in.readString("value"));
      setOutputRange(value);
      return;
    }

    if (name == "attenuation")
    {
      auto value = in.readDouble("value");
      setAttenutation(value);
      return;
    }


    ThrowException("internal error");

  }

  UndoableModel::executeAction(in);
}


////////////////////////////////////////////////////////////////////
void TransferFunction::addFunction(SharedPtr<SingleTransferFunction> fn)
{
  if (!functions.empty())
    fn->setNumberOfSamples(getNumberOfSamples());

  auto nsamples = fn->values.size();

  pushAction(
    fn->encode("AddFunction"),
    fullUndo());
  {
    this->default_name = "";
    functions.push_back(fn);
  }
  popAction();
}



////////////////////////////////////////////////////////////////////
void TransferFunction::removeFunction(int index)
{
  auto fn = functions[index];

  pushAction(
    StringTree("RemoveFunction").write("index", index),
    fullUndo());
  {
    this->default_name = "";
    this->functions.erase(this->functions.begin() + index);
  }
  popAction();
}

////////////////////////////////////////////////////////////////////
void TransferFunction::clearFunctions()
{
  if (functions.empty())
    return;

  pushAction(
    StringTree("ClearFunctions"),
    fullUndo());
  {
    this->default_name = "";
    this->functions.clear();
  }
  popAction();
}

////////////////////////////////////////////////////////////////////////////////
void TransferFunction::setNumberOfSamples(int value)
{
  if (value == this->getNumberOfSamples())
    return;

  pushAction(
    StringTree("SetProperty").write("name","num_samples").write("value", cstring(value)),
    fullUndo());
  {
    for (auto fn : functions)
      fn->setNumberOfSamples(value);
  }
  popAction();
}

////////////////////////////////////////////////////////////////////////////////
void TransferFunction::setNumberOfFunctions(int value)
{
  if (getNumberOfSamples() == value)
    return;

  pushAction(
    StringTree("SetProperty").write("name","num_functions").write("value", value),
    fullUndo());
  {
    this->functions.resize(std::min(value,this->getNumberOfFunctions()));

    while (getNumberOfFunctions() < value)
    {
      int nsamples = functions.empty()? 256 : getNumberOfSamples();
      addFunction(std::make_shared<SingleTransferFunction>(guessName(getNumberOfFunctions()), guessColor(getNumberOfFunctions()), std::vector<double>(nsamples, 0.0)));
    }
  }
  popAction();
}


/////////////////////////////////////////////////////////////////////
TransferFunction& TransferFunction::operator=(const TransferFunction& other)
{
  pushAction(
    EncodeObject(&other,"Assign"),
    fullUndo());
  {
    this->default_name = other.default_name;

    this->attenuation = other.attenuation;
    this->input_range = other.input_range;
    this->output_dtype = other.output_dtype;
    this->output_range = other.output_range;

    this->functions.clear();
    for (auto fn : other.functions)
      this->functions.push_back(std::make_shared<SingleTransferFunction>(*fn));
  }
  popAction();

  return *this;
}


////////////////////////////////////////////////////////////////////
void TransferFunction::drawLine(Point2d p1, Point2d p2, std::vector<int> selected)
{
  if (selected.empty())
    return;

  if (p2.x < p1.x)
    std::swap(p1, p2);

  int N = (int)getNumberOfSamples();
  p1.x = Utils::clamp(p1.x, 0.0, 1.0); p1.y = Utils::clamp(p1.y, 0.0, 1.0);
  p2.x = Utils::clamp(p2.x, 0.0, 1.0); p2.y = Utils::clamp(p2.y, 0.0, 1.0);

  int i_x1 = Utils::clamp((int)(round(p1.x * (N - 1))), 0, N - 1);
  int i_x2 = Utils::clamp((int)(round(p2.x * (N - 1))), 0, N - 1);

  pushAction(
    StringTree("DrawLine").write("p1", p1).write("p2", p2).write("selected", StringUtils::join(selected)),
    fullUndo());
  {
    this->default_name = "";

    for (int F = 0; F < (int)functions.size(); F++)
    {
      auto fn = functions[F];
      if (Utils::find(selected, F) < 0)
        continue;

      fn->values[i_x1] = p1.y;
      fn->values[i_x2] = p2.y;

      //interpolate if needed
      for (int K = i_x1 + 1; K < i_x2; K++)
      {
        double beta = (K - i_x1) / (double)(i_x2 - i_x1);
        double alpha = (1 - beta);
        fn->values[K] = alpha * p1.y + beta * p2.y;
      }
    }
  }
  popAction();
}


////////////////////////////////////////////////////////////////////////////////
void TransferFunction::setInputRange(ComputeRange new_value)
{
  auto old_value = this->input_range;
  if (old_value == new_value) return;

  pushAction(
    StringTree("SetProperty").write("name", String("input_range")).write("mode", cstring(new_value.mode)).write("custom_range", new_value.custom_range.toString()),
    StringTree("SetProperty").write("name", String("input_range")).write("mode", cstring(old_value.mode)).write("custom_range", old_value.custom_range.toString()));
  {
    this->input_range = new_value;
  }
  popAction();
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
TransferFunction TransferFunction::importTransferFunction(String url)
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
    return TransferFunction();
  }

  int nsamples = cint(lines[0]);
  int nfunctions = 4;
  lines.erase(lines.begin());
  if (lines.size() != nsamples)
  {
    VisusWarning() << "content is of incorrect length";
    return TransferFunction();
  }

  TransferFunction ret;
  ret.output_dtype=DTypes::UINT8;
  ret.output_range=Range(0, 255, 1);

  for (int F = 0; F < nfunctions; F++)
    ret.functions.push_back(std::make_shared<SingleTransferFunction>(guessName(F), guessColor(F), std::vector<double>(nsamples, 0.0)));

  for (int I = 0; I < nsamples; I++)
  {
    std::istringstream istream(lines[I]);

    for (int F = 0; F < nfunctions; F++)
    {
      double value;
      istream >> value;
      value = value / (nsamples - 1.0);
      ret.functions[F]->values[I] = value;
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
void TransferFunction::writeTo(StringTree& out) const
{
  bool bDefault=default_name.empty()?false:true;

  if (isDefault())
    out.write("name", default_name);

  out.write("nsamples", getNumberOfSamples());

  out.write("attenuation",cstring(attenuation));
    
  out.write("input/mode", cstring(input_range.mode));
  out.write("input/custom_range", input_range.custom_range);

  out.write("output/dtype", output_dtype.toString());
  out.write("output/range", output_range);

  if (!isDefault())
  {
    for (auto fn : functions)
      fn->writeTo(*out.addChild("function"));
  }
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::readFrom(StringTree& in)
{
  this->functions.clear();

  this->default_name = in.readString("name");
  bool is_default = default_name.empty() ? false : true;

  int nsamples = in.readInt("nsamples");

  if (is_default)
    (*this) = *getDefault(default_name, nsamples);

  this->attenuation=cdouble(in.readString("attenuation","0.0"));

  this->input_range.mode=(ComputeRange::Mode)cint(in.readString("input/mode"));
  this->input_range.custom_range=Range::fromString(in.read("input/custom_range"));

  this->output_dtype=DType::fromString(in.readString("output/dtype"));
  this->output_range=Range::fromString(in.read("output/range"));

  if (!is_default)
  {
    for (auto Function : in.getChilds("function"))
    {
      auto single = std::make_shared<SingleTransferFunction>();
      single->readFrom(*Function);
      functions.push_back(single);
    }
  }
}

} //namespace Visus

