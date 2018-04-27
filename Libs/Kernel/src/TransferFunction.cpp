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


/////////////////////////////////////////////////////////////////
RGBAColorMap::RGBAColorMap(String name_ ,const double* values,size_t num) : name(name_)
{
  VisusAssert(!name.empty());

  this->points.clear();
  
  for (size_t I=0;I<num;I+=4,values+=4)
  {
    double x=values[0];
    double r=values[2];
    double g=values[3];
    double b=values[4];
    double a=1;
    VisusAssert(points.empty() || points.back().x<=x);

    Point point;
    point.x=x;
    point.color=Color((float)r,(float)g,(float)b,(float)a);
    this->points.push_back(point);
  }
  VisusAssert(!points.empty());
  refreshMinMax(); 
}


/////////////////////////////////////////////////////////////////
Color RGBAColorMap::getColor(double alpha,InterpolationMode::Type type)
{
  VisusAssert(alpha>=0 && alpha<=1);
  double x = min_x+alpha*(max_x-min_x);

  for (int I=0;I<(int)(points.size()-1);I++)
  {
    const Point& p0=points[I+0];
    const Point& p1=points[I+1];
      
    if (p0.x<=x && x<=p1.x)
    {
      if (type==InterpolationMode::FLAT) 
        return p0.color;

      alpha=(x-p0.x)/(p1.x-p0.x);
      if (type==InterpolationMode::INVERTED)
        alpha=1-alpha;

      Color c0=p0.color.toCieLab();
      Color c1=p1.color.toCieLab();
      Color ret=Color::interpolate((Float32)(1-alpha),c0,(Float32)alpha,c1).toRGB();
      return ret;
    }
  }

  VisusAssert(false);
  return Colors::Black;
}

/////////////////////////////////////////////////////////////////
void RGBAColorMap::convertToArray(Array& dst,int nsamples,InterpolationMode::Type interpolation)
{
  dst.resize(nsamples,DTypes::UINT8_RGBA,__FILE__,__LINE__);

  Uint8* DST=dst.c_ptr();
  for (int I=0;I<nsamples;I++)
  {
    double alpha=I/(double)(nsamples-1);
    Color color=getColor(alpha,interpolation);
    *DST++=(Uint8)(255*color.getRed  ());
    *DST++=(Uint8)(255*color.getGreen());
    *DST++=(Uint8)(255*color.getBlue ());
    *DST++=(Uint8)(255*color.getAlpha());
  }
}

/////////////////////////////////////////////////////////////////
void RGBAColorMap::writeToObjectStream(ObjectStream& ostream)
{
  ostream.writeInline("name",this->name);
  for (int I=0;I<(int)points.size();I++)
  {
    const Point& point=points[I];
    ostream.pushContext("Point");
    ostream.writeInline("x",cstring(point.x));
    ostream.writeInline("r",cstring(point.color.getRed()));
    ostream.writeInline("g",cstring(point.color.getGreen()));
    ostream.writeInline("b",cstring(point.color.getBlue()));
    ostream.writeInline("o",cstring(point.color.getAlpha()));
    ostream.popContext("Point");
  }

}

/////////////////////////////////////////////////////////////////
void RGBAColorMap::readFromObjectStream(ObjectStream& istream)
{
  this->name=istream.readInline("name");
  VisusAssert(!name.empty());

  this->points.clear();
  while (istream.pushContext("Point"))
  {
    double x=cdouble(istream.readInline("x"));
    double o=cdouble(istream.readInline("o"));
    double r=cdouble(istream.readInline("r"));
    double g=cdouble(istream.readInline("g"));
    double b=cdouble(istream.readInline("b"));
    VisusAssert(points.empty() || points.back().x<=x);

    Point point;
    point.x=x;
    point.color=Color((float)r,(float)g,(float)b,(float)o);
        
    this->points.push_back(point);
    istream.popContext("Point");
  }
  VisusAssert(!points.empty());
  refreshMinMax();
}


////////////////////////////////////////////////////////////////////
void TransferFunction::copy(TransferFunction& dst,const TransferFunction& src)
{
  if (&dst==&src)
    return;

  dst.beginUpdate();
  {
    dst.input.normalization=src.input.normalization;
    dst.output.dtype=src.output.dtype;
    dst.output.range=src.output.range;
    dst.default_name=src.default_name;
    dst.attenuation=src.attenuation;

    dst.functions.clear();
    for (auto fn : src.getFunctions())
      dst.functions.push_back(std::make_shared<Function>(*fn));
  }
  dst.endUpdate();
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::setInputNormalization(InputNormalization value)
{
  beginUpdate();
  this->input.normalization=value;
  endUpdate();
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::setAttenuation(double value)
{
  if (this->attenuation==value)
    return;

  beginUpdate();
  this->attenuation=value;
  endUpdate();
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::setOutputDType(DType value)
{
  if (this->output.dtype==value)
    return;

  beginUpdate();
  this->output.dtype=value;
  output.range=output.dtype==DTypes::UINT8? Range(0,255,1) : Range(0,1,0);
  endUpdate();
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::setOutputRange(Range value)
{
  if (this->output.range.from==value.from && this->output.range.to==value.to)
    return;
 
  beginUpdate();
  this->output.range=value;
  endUpdate();
}


/////////////////////////////////////////////////////////////////////
void TransferFunction::resize(int value)
{
  if (size()==value || !value || !getNumberOfFunctions())
    return;

  beginUpdate();
  for (auto fn : getFunctions())
    fn->resize(value);
  endUpdate();
}

/////////////////////////////////////////////////////////////////////
String TransferFunction::guessFunctionName()
{
  int N=getNumberOfFunctions();
  switch (N)
  {
    case 0:return "Red"  ;
    case 1:return "Green";
    case 2:return "Blue" ;
    case 3:return "Alpha";
    default:return cstring(N);
  }
}

/////////////////////////////////////////////////////////////////////
Color TransferFunction::guessFunctionColor()
{
  int N=getNumberOfFunctions();
  switch (N)
  {
    case 0:return Colors::Red;
    case 1:return Colors::Green;
    case 2:return Colors::Blue;
    case 3:return Colors::Gray;
    default:return Color::random();
  }
}


////////////////////////////////////////////////////////////////////
void TransferFunction::setNumberOfFunctions(int value)
{
  value=std::max(1,value);
  int nsamples=size() ;

  beginUpdate();
  {
    while (getNumberOfFunctions()<value)
      functions.push_back(std::make_shared<SingleTransferFunction>(guessFunctionName(),guessFunctionColor(),nsamples));
      
    while (getNumberOfFunctions()>value)
      functions.pop_back();
  }
  endUpdate();
}


/////////////////////////////////////////////////////////////////////
Array TransferFunction::convertToArray() const
{
  int nfun =(int)getNumberOfFunctions();
  if (!nfun)
    return Array();

  Array ret;

  int nsamples=size();

  std::vector<double> alpha(nfun,1.0);
  for (int F=0;F<nfun;F++) 
  {
    //RGBA palette
    if (attenuation && nfun==4 && F==3)
      alpha[F]=1.0-attenuation;

    //luminance+alpha
    else if (attenuation && nfun==2 && F==1)
      alpha[F]=1.0-attenuation;
  }

  double vs=output.range.delta();
  double vt=output.range.from;

  if (output.dtype==DTypes::UINT8)
  {
    if (!ret.resize(nsamples,DType(nfun,DTypes::UINT8),__FILE__,__LINE__)) 
      return Array();

    for (int F=0;F<nfun;F++)
    {
      auto fn=getFunction(F);
      GetComponentSamples<Uint8> write(ret,F);
      for (int I=0;I<nsamples;I++)
        write[I]=(Uint8)((alpha[F]*fn->values[I])*vs+vt);
    }
  }
  else
  {
    if (!ret.resize(nsamples,DType(nfun,DTypes::FLOAT32),__FILE__,__LINE__)) 
      return Array();

    for (int F=0;F<nfun;F++)
    {
      auto fn=getFunction(F);
      GetComponentSamples<Float32> write(ret,F);
      for (int I=0;I<nsamples;I++)
        write[I]=(Float32)((alpha[F]*fn->values[I])*vs+vt);
    }
  }

  return ret;
}

///////////////////////////////////////////////////////////
bool TransferFunction::importTransferFunction(String url)
{
  if (url.empty())
    return false;

  String content=Utils::loadTextDocument(url);

  std::vector<String> lines=StringUtils::getNonEmptyLines(content);
  if (lines.empty())  
  {
    VisusWarning()<<"content is empty"; 
    return false;
  }
  
  int size=cint(lines[0]);
  lines.erase(lines.begin());
  if (lines.size()!=size) 
  {
    VisusWarning()<<"content is of incorrect length";
    return false;
  }

  beginUpdate();
  {
    this->functions.clear();
    this->functions.push_back(std::make_shared<SingleTransferFunction>("Red"  ,Colors::Red  ,size));
    this->functions.push_back(std::make_shared<SingleTransferFunction>("Green",Colors::Green,size));
    this->functions.push_back(std::make_shared<SingleTransferFunction>("Blue" ,Colors::Blue ,size));
    this->functions.push_back(std::make_shared<SingleTransferFunction>("Alpha",Colors::Gray ,size));

    double N=size-1.0;
    for (int I=0;I<size;I++)
    {
      std::istringstream istream(lines[I]);
      for (auto fn : getFunctions())
      {
        int value;istream>>value;
        fn->values[I]=value/N;
      }
    }

    this->output.dtype=DTypes::UINT8;
    this->output.range=Range(0,255,1);
    this->attenuation=0.0;
  }
  endUpdate();

  return true;
}

///////////////////////////////////////////////////////////
bool TransferFunction::exportTransferFunction(String filename="")
{
  int nsamples = size();

  if (!nsamples) 
    return false;

  std::ostringstream out;
  out<<nsamples<<std::endl;
  for (int I=0;I<nsamples;I++)
  {
    for (auto fn : getFunctions())
      out<<(int)(fn->values[I]*(nsamples-1))<<" ";
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
bool TransferFunction::setFromArray(Array src,String default_name)
{
  int nfunctions =src.dtype.ncomponents();
  int N          =(int)src.dims[0];

  beginUpdate();
  {
    functions.clear();
    for (int F=0;F<nfunctions;F++) 
      addFunction(std::make_shared<SingleTransferFunction>(guessFunctionName(),guessFunctionColor(),N));
  
    if (src.dtype.isVectorOf(DTypes::UINT8)) 
    {
      const Uint8* SRC=src.c_ptr();
      for (int I=0;I<N;I++)
      {
        for (int F=0;F<nfunctions;F++)
          getFunction(F)->values[I]=(*SRC++)/255.0;
      }

      this->default_name=default_name;
    }
    else if (src.dtype.isVectorOf(DTypes::FLOAT32)) 
    {
      const Float32* SRC=(const Float32*)src.c_ptr();
      for (int I=0;I<N;I++)
      {
        for (int F=0;F<nfunctions;F++)
          getFunction(F)->values[I]=(*SRC++);
      }

      this->default_name=default_name;
    }
    else if (src.dtype.isVectorOf(DTypes::FLOAT64)) 
    {
      const Float64* SRC=(const Float64*)src.c_ptr();
      for (int I=0;I<N;I++)
      {
        for (int F=0;F<nfunctions;F++)
          getFunction(F)->values[I]=(*SRC++);
      }

      this->default_name=default_name;
    }
    else
    {
      VisusAssert(false);
    }

  }
  endUpdate();
  return true;
}



/////////////////////////////////////////////////////////////////////
template <typename SrcType>
struct ExecuteProcessingInnerOp
{
  template <typename DstType>
  bool execute(TransferFunction& tf, Array& dst, Array src, Aborted aborted)
  {
    int num_fn = (int)tf.getNumberOfFunctions();
    if (!num_fn)
      return false;

    int src_ncomponents = (int)src.dtype.ncomponents();
    if (!src_ncomponents)
      return false;

    DType dst_dtype = tf.getOutputDType();
    if (!(dst_dtype == DTypes::UINT8 || dst_dtype == DTypes::FLOAT32 || dst_dtype == DTypes::FLOAT64))
    {
      VisusAssert(false);
      return false;
    }

    Int64 tot = src.getTotalNumberOfSamples();

    //example: f(a,b,c)   -> f(a) f(b) f(c)
    int dst_ncomponents = 0;
    if (num_fn == 1)
      dst_ncomponents = src_ncomponents;

    //example: (f,g,h)(a) -> f(a) g(a) h(a)
    else if (src_ncomponents == 1)
      dst_ncomponents = num_fn;
    
    else
      dst_ncomponents = std::min(num_fn, src_ncomponents);

    if (!dst_ncomponents)
      return false;

    dst_dtype = DType(dst_ncomponents, dst_dtype);

    if (!dst.resize(src.dims, dst_dtype, __FILE__, __LINE__))
      return false;

    for (int I = 0; I < dst_ncomponents; I++)
    {
      auto fn = tf.getFunction(std::min(I, num_fn-1));
      auto dst_write = GetComponentSamples<DstType>(dst, I);
      auto src_read  = GetComponentSamples<SrcType>(src, std::min(I, src_ncomponents-1));

      dst.dtype=dst.dtype.withDTypeRange(tf.getOutputRange(),I);

      auto  vs_t = tf.getInputNormalization().doCompute(src, src_read.C,aborted).getScaleTranslate();

      Range dst_range = tf.getOutputRange();

      double src_vs = vs_t.first;
      double src_vt = vs_t.second;

      double dst_vs = dst_range.delta();
      double dst_vt = dst_range.from;

      for (int I = 0; I < tot; I++)
      {
        if (aborted())
          return false;

        double x = src_vs*src_read[I] + src_vt;
        double y = fn->getValue(x);
        dst_write[I] = (DstType)(dst_vs*y + dst_vt);
      }
    }

    dst.shareProperties(src);
    return true;
  }
};


struct ExecuteProcessingOp
{
  template <typename SrcType>
  bool execute(TransferFunction& tf, Array& dst, Array src, Aborted aborted) {
    ExecuteProcessingInnerOp<SrcType> op;
    return ExecuteOnCppSamples(op, tf.getOutputDType(), tf, dst, src, aborted);
  }
};


Array TransferFunction::applyToArray(Array src,Aborted aborted)
{
  Array dst;
  ExecuteProcessingOp op;
  return ExecuteOnCppSamples(op,src.dtype,*this,dst,src,aborted)? dst : Array();
}

/////////////////////////////////////////////////////////////////////
void SingleTransferFunction::writeToObjectStream(ObjectStream& ostream)
{
  ostream.write("name",name);
  ostream.write("color",color.toString());

  ostream.pushContext("values");
  {
    std::ostringstream out;
    for (int I=0;I<(int)values.size();I++) 
    {
      if (I % 16==0) out<<std::endl;
      out<<values[I]<<" ";
    }
    ostream.writeText(out.str());
  }
  ostream.popContext("values");
}

/////////////////////////////////////////////////////////////////////
void SingleTransferFunction::readFromObjectStream(ObjectStream& istream)
{
  name=istream.read("name");
  color=Color::parseFromString(istream.read("color"));

  this->values.clear();
  istream.pushContext("values");
  {
    std::istringstream in(istream.readText());
    double value;while (in>>value)
      values.push_back(value);
  }
  istream.popContext("values");
}


/////////////////////////////////////////////////////////////////////
void TransferFunction::writeToObjectStream(ObjectStream& ostream)
{
  bool bDefault=default_name.empty()?false:true;

  if (bDefault)
    ostream.writeInline("name",default_name);

  ostream.writeInline("attenuation",cstring(attenuation));

  ostream.pushContext("input");
  {
    ostream.writeInline("mode",cstring(input.normalization.mode));
    if (input.normalization.custom_range.delta()>0)
    {
      ostream.pushContext("custom_range");
      input.normalization.custom_range.writeToObjectStream(ostream);
      ostream.popContext("custom_range");
    }
  }
  ostream.popContext("input");

  ostream.pushContext("output");
  {
    ostream.writeInline("dtype",output.dtype.toString());
    ostream.pushContext("range");
    output.range.writeToObjectStream(ostream);
    ostream.popContext("range");
  }
  ostream.popContext("output");

  if (!bDefault)
  {
    for (auto fn : getFunctions())
    {
      ostream.pushContext("function");
      fn->writeToObjectStream(ostream);
      ostream.popContext("function");
    }
  }
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::readFromObjectStream(ObjectStream& istream)
{
  this->functions.clear();
  
  this->default_name=istream.readInline("name");

  bool bDefault=default_name.empty()?false:true;

  this->attenuation=cdouble(istream.readInline("attenuation","0.0"));

  if (istream.pushContext("input"))
  {
    input.normalization.mode=(ComputeRange::Mode)cint(istream.readInline("input.normalization"));
    if (istream.pushContext("custom_range"))
    {
      input.normalization.custom_range.readFromObjectStream(istream);
      istream.popContext("custom_range");
    }
    istream.popContext("input");
  }

  if (istream.pushContext("output"))
  {
    output.dtype=DType::fromString(istream.readInline("dtype"));

    if (istream.pushContext("range"))
    {
      output.range.readFromObjectStream(istream);
      istream.popContext("range");
    }
    istream.popContext("output");
  }

  if (bDefault)
  {
     setDefault(default_name);
  }
  else
  {
    setNotDefault();
    while (istream.pushContext("function"))
    {
      auto fn= std::make_shared<SingleTransferFunction>();
      fn->readFromObjectStream(istream);
      istream.popContext("function");
      addFunction(fn);
    }
  }
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::writeToSceneObjectStream(ObjectStream& ostream)
{
  ostream.pushContext("palette");
  ostream.writeInline("attenuation",cstring(attenuation));
  
  ostream.pushContext("output");
  {
    ostream.writeInline("dtype",output.dtype.toString());
    ostream.pushContext("range");
    output.range.writeToObjectStream(ostream);
    ostream.popContext("range");
  }
  ostream.popContext("output");
  
  for (auto fn : getFunctions())
  {
    ostream.pushContext("function");
    fn->writeToObjectStream(ostream);
    ostream.popContext("function");
  }
  ostream.popContext("palette");
}
  
/////////////////////////////////////////////////////////////////////
void TransferFunction::readFromSceneObjectStream(ObjectStream& istream)
{
  this->functions.clear();
  
  this->default_name=istream.readInline("name");
  
  bool bDefault=default_name.empty()?false:true;
  
  this->attenuation=cdouble(istream.readInline("attenuation","0.0"));
  
  if (istream.pushContext("input"))
  {
    input.normalization.mode=(ComputeRange::Mode)cint(istream.readInline("input.normalization"));
    if (istream.pushContext("custom_range"))
    {
      input.normalization.custom_range.readFromObjectStream(istream);
      istream.popContext("custom_range");
    }
    istream.popContext("input");
  }
  
  if (istream.pushContext("output"))
  {
    output.dtype=DType::fromString(istream.readInline("dtype"));
    
    if (istream.pushContext("range"))
    {
      output.range.readFromObjectStream(istream);
      istream.popContext("range");
    }
    istream.popContext("output");
  }
  
  if (bDefault)
  {
    setDefault(default_name);
  }
  else
  {
    setNotDefault();
    while (istream.pushContext("function"))
    {
      auto fn= std::make_shared<SingleTransferFunction>();
      fn->readFromObjectStream(istream);
      istream.popContext("function");
      addFunction(fn);
    }
  }
}

} //namespace Visus

