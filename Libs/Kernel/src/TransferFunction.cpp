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
      if (type==InterpolationMode::Flat) 
        return p0.color;

      alpha=(x-p0.x)/(p1.x-p0.x);
      if (type==InterpolationMode::Inverted)
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
  ostream.writeString("name",this->name);
  for (int I=0;I<(int)points.size();I++)
  {
    const Point& point=points[I];
    ostream.pushContext("Point");
    ostream.writeString("x",cstring(point.x));
    ostream.writeString("r",cstring(point.color.getRed()));
    ostream.writeString("g",cstring(point.color.getGreen()));
    ostream.writeString("b",cstring(point.color.getBlue()));
    ostream.writeString("o",cstring(point.color.getAlpha()));
    ostream.popContext("Point");
  }

}

/////////////////////////////////////////////////////////////////
void RGBAColorMap::readFromObjectStream(ObjectStream& istream)
{
  this->name=istream.readString("name");
  VisusAssert(!name.empty());

  this->points.clear();
  while (istream.pushContext("Point"))
  {
    double x=cdouble(istream.readString("x"));
    double o=cdouble(istream.readString("o"));
    double r=cdouble(istream.readString("r"));
    double g=cdouble(istream.readString("g"));
    double b=cdouble(istream.readString("b"));
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



/////////////////////////////////////////////////////////////////////
void TransferFunction::Single::writeToObjectStream(ObjectStream& ostream)
{
  ostream.writeValue("name", name);
  ostream.writeValue("color", color.toString());

  ostream.pushContext("values");
  {
    std::ostringstream out;
    for (int I = 0; I<(int)values.size(); I++)
    {
      if (I % 16 == 0) out << std::endl;
      out << values[I] << " ";
    }
    ostream.writeText(out.str());
  }
  ostream.popContext("values");
}

/////////////////////////////////////////////////////////////////////
void TransferFunction::Single::readFromObjectStream(ObjectStream& istream)
{
  name = istream.readValue("name");
  color = Color::parseFromString(istream.readValue("color"));

  this->values.clear();

  std::istringstream in(istream.readText("values"));
  double value; while (in >> value)
    values.push_back(value);
}

////////////////////////////////////////////////////////////////////
SharedPtr<TransferFunction::Single> TransferFunction::addFunction(String name, Color color, int nsamples) 
{
  auto ret = functions.empty() ? std::make_shared<Single>(nsamples) : std::make_shared<Single>(size());
  ret->name = name;
  ret->color = color;

  beginUpdate();
  functions.push_back(ret);
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////
void TransferFunction::setNumberOfFunctions(int value,std::vector<String> names,std::vector<Color> colors)
{
  value = std::max(1, value);
  if (value == functions.size()) return;

  beginUpdate();
  {
    while (functions.size() < value)
    {
      auto single = functions.empty() ? std::make_shared<Single>() : std::make_shared<Single>(size());
      single->name =  functions.size()<names.size() ? names[functions.size()] : cstring((int)functions.size());
      single->color = functions.size()<colors.size() ? colors[functions.size()] : Color::random();
      functions.push_back(single);
    }

    while (functions.size()>value)
      functions.pop_back();
  }
  endUpdate();
}

////////////////////////////////////////////////////////////////////
void TransferFunction::copy(TransferFunction& dst,const TransferFunction& src)
{
  if (&dst==&src)
    return;

  dst.beginUpdate();
  {
    dst.input_range=src.input_range;
    dst.output_dtype=src.output_dtype;
    dst.output_range=src.output_range;
    dst.default_name=src.default_name;
    dst.attenuation=src.attenuation;

    dst.functions.clear();
    for (auto fn : src.functions)
      dst.functions.push_back(std::make_shared<Single>(*fn));
  }
  dst.endUpdate();
}




/////////////////////////////////////////////////////////////////////
Array TransferFunction::convertToArray() const
{
  int nfun =(int)functions.size();
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

  double vs=output_range.delta();
  double vt=output_range.from;

  if (output_dtype==DTypes::UINT8)
  {
    if (!ret.resize(nsamples,DType(nfun,DTypes::UINT8),__FILE__,__LINE__)) 
      return Array();

    for (int F=0;F<nfun;F++)
    {
      auto fn=functions[F];
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
      auto fn=functions[F];
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
  
  int nsamples=cint(lines[0]);
  lines.erase(lines.begin());
  if (lines.size()!= nsamples)
  {
    VisusWarning()<<"content is of incorrect length";
    return false;
  }

  beginUpdate();
  {
    this->functions.clear();
    addFunction("Red"  ,Colors::Red  , nsamples);
    addFunction("Green",Colors::Green, nsamples);
    addFunction("Blue" ,Colors::Blue , nsamples);
    addFunction("Alpha",Colors::Gray , nsamples);

    double N= nsamples -1.0;
    for (int I=0;I<nsamples;I++)
    {
      std::istringstream istream(lines[I]);
      for (auto fn : functions)
      {
        int value;istream>>value;
        fn->values[I]=value/N;
      }
    }

    this->output_dtype=DTypes::UINT8;
    this->output_range=Range(0,255,1);
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
    for (auto fn : functions)
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
bool TransferFunction::setFromArray(Array src,String default_name,std::vector<String> names,std::vector<Color> colors)
{
  int nfunctions =src.dtype.ncomponents();
  int N          =(int)src.dims[0];

  beginUpdate();
  {
    functions.clear();
    for (int F = 0; F < nfunctions; F++)
    {
      auto single = std::make_shared<Single>(N);
      single->name  = functions.size()<names.size() ? names[functions.size()] : cstring((int)functions.size());
      single->color = functions.size()<colors.size() ? colors[functions.size()] : Color::random();
      functions.push_back(single);
    }
  
    if (src.dtype.isVectorOf(DTypes::UINT8)) 
    {
      const Uint8* SRC=src.c_ptr();
      for (int I=0;I<N;I++)
      {
        for (int F=0;F<nfunctions;F++)
          functions[F]->values[I]=(*SRC++)/255.0;
      }

      this->default_name=default_name;
    }
    else if (src.dtype.isVectorOf(DTypes::FLOAT32)) 
    {
      const Float32* SRC=(const Float32*)src.c_ptr();
      for (int I=0;I<N;I++)
      {
        for (int F=0;F<nfunctions;F++)
          functions[F]->values[I]=(*SRC++);
      }

      this->default_name=default_name;
    }
    else if (src.dtype.isVectorOf(DTypes::FLOAT64)) 
    {
      const Float64* SRC=(const Float64*)src.c_ptr();
      for (int I=0;I<N;I++)
      {
        for (int F=0;F<nfunctions;F++)
          functions[F]->values[I]=(*SRC++);
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
    int num_fn = (int)tf.functions.size();
    if (!num_fn)
      return false;

    int src_ncomponents = (int)src.dtype.ncomponents();
    if (!src_ncomponents)
      return false;

    DType dst_dtype = tf.output_dtype;
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
      auto F = Utils::clamp(I, 0, num_fn          - 1); auto FUN = tf.functions[F];
      auto D = Utils::clamp(I, 0, dst_ncomponents - 1); auto DST = GetComponentSamples<DstType>(dst, D);
      auto S = Utils::clamp(I, 0, src_ncomponents - 1); auto SRC = GetComponentSamples<SrcType>(src, S);

      dst.dtype=dst.dtype.withDTypeRange(tf.output_range,I);
      auto  vs_t = tf.input_range.doCompute(src, SRC.C, aborted).getScaleTranslate();

      Range dst_range = tf.output_range;

      double src_vs = vs_t.first;
      double src_vt = vs_t.second;

      double dst_vs = dst_range.delta();
      double dst_vt = dst_range.from;

      for (int I = 0; I < tot; I++)
      {
        if (aborted())
          return false;

        double x = src_vs* SRC[I] + src_vt;
        double y = FUN->getValue(x);
        DST[I] = (DstType)(dst_vs*y + dst_vt);
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
    return ExecuteOnCppSamples(op, tf.output_dtype, tf, dst, src, aborted);
  }
};


Array TransferFunction::applyToArray(Array src,Aborted aborted)
{
  if (!src)
    return src;

  Array dst;
  ExecuteProcessingOp op;
  return ExecuteOnCppSamples(op,src.dtype,*this,dst,src,aborted)? dst : Array();
}


/////////////////////////////////////////////////////////////////////
void TransferFunction::writeToObjectStream(ObjectStream& ostream)
{
  bool bDefault=default_name.empty()?false:true;

  if (bDefault)
    ostream.writeString("name",default_name);

  ostream.writeString("attenuation",cstring(attenuation));

  ostream.pushContext("input");
  {
    ostream.writeString("mode",cstring(input_range.mode));
    if (input_range.custom_range.delta()>0)
    {
      ostream.pushContext("custom_range");
      input_range.custom_range.writeToObjectStream(ostream);
      ostream.popContext("custom_range");
    }
  }
  ostream.popContext("input");

  ostream.pushContext("output");
  {
    ostream.writeString("dtype",output_dtype.toString());
    ostream.pushContext("range");
    output_range.writeToObjectStream(ostream);
    ostream.popContext("range");
  }
  ostream.popContext("output");

  if (!bDefault)
  {
    for (auto fn : functions)
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
  
  this->default_name=istream.readString("name");

  bool bDefault=default_name.empty()?false:true;

  this->attenuation=cdouble(istream.readString("attenuation","0.0"));

  if (istream.pushContext("input"))
  {
    input_range.mode=(ComputeRange::Mode)cint(istream.readString("input.normalization"));
    if (istream.pushContext("custom_range"))
    {
      input_range.custom_range.readFromObjectStream(istream);
      istream.popContext("custom_range");
    }
    istream.popContext("input");
  }

  if (istream.pushContext("output"))
  {
    output_dtype=DType::fromString(istream.readString("dtype"));

    if (istream.pushContext("range"))
    {
      output_range.readFromObjectStream(istream);
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
      auto single = std::make_shared<Single>();
      single->readFromObjectStream(istream);
      istream.popContext("function");
      functions.push_back(single);
    }
  }
}



} //namespace Visus

