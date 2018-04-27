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

#ifndef VISUS_TRANSFER_FUNCTION_H
#define VISUS_TRANSFER_FUNCTION_H

#include <Visus/Kernel.h>
#include <Visus/Model.h>
#include <Visus/Color.h>
#include <Visus/Array.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API InterpolationMode
{
public:

  VISUS_CLASS(InterpolationMode)

  enum Type {
    DEFAULT,
    FLAT,
    INVERTED};

  //InterpolationMode
  InterpolationMode()  {
  }

  //toString
  String toString() const
  {
    switch (type)
    {
      case FLAT    : return "Flat";
      case INVERTED: return "Inverted";
      case DEFAULT :
      default      : return "Default";
    }
  }

  //set
  void set(const String &name)
  {
    if      (name=="Default") type=DEFAULT;
    else if (name=="Flat") type=FLAT;
    else if (name=="Inverted") type=INVERTED;
  }

  //get
  Type get() { 
    return type; 
  }

private:

  Type type=DEFAULT;

};


/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RGBAColorMap 
{
public:

  VISUS_CLASS(RGBAColorMap)

  class VISUS_KERNEL_API Point
  {
  public:
    double x;
    Color color;
    inline Point() : x(0) {}
    inline Point(double x_,Color color_) : x(x_),color(color_) {}
  };

  String            name;
  double            min_x=0,max_x=0;
  std::vector<Point> points;

  //constructor
  RGBAColorMap() {
  }

  //constructor
  RGBAColorMap(String name,const double* values,size_t num);

  //destructor
  ~RGBAColorMap() {
  }

  //refreshMinMax
  void refreshMinMax() {
    min_x=NumericLimits<double>::highest();
    max_x=NumericLimits<double>::lowest ();
    for (int I=0;I<(int)points.size();I++)
    {
      min_x=std::min(min_x,points[I].x);
      max_x=std::max(max_x,points[I].x);
    }
  }

  //getColor
  Color getColor(double alpha,InterpolationMode::Type type=InterpolationMode::DEFAULT);

  //convertToArray
  void convertToArray(Array& dst,int nsamples,InterpolationMode::Type type=InterpolationMode::DEFAULT);

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream);

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream);

};


////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API SingleTransferFunction 
{
public:

  VISUS_CLASS(SingleTransferFunction)

  String  name;
  Color   color;
  std::vector<double> values;

  //constructor (identity function)
  SingleTransferFunction(String name_ = "", Color color_ = Colors::Black, int nsamples = 256): name(name_),color(color_)
  {
    for (int I=0;I<nsamples;I++)
      values.push_back(I/(double)(nsamples-1));
  }

  //destructor
  virtual ~SingleTransferFunction() {
  }

  //size
  inline int size() const {
    return (int)values.size();
  }

  //resize
  void resize(int value)
  {
    if (value==this->size())
      return;

    std::vector<double> values(value);
    for (int I=0;I<value;I++)
    {
      double alpha=I/(double)(value-1);
      values[I]=this->getValue(alpha);
    }

    this->values=values;
  }

  //getValue (x must be in range [0,1])
  double getValue(double x) const
  {
    int N=size();
    if (!N) {
      VisusAssert(false);
      return 0;
    }

    x=Utils::clamp(x*(N-1),0.0,N-1.0);

    int i_x1=Utils::clamp((int)std::floor(x),0,N-1);
    int i_x2=Utils::clamp((int)std::ceil (x),0,N-1);

    if (i_x1==i_x2) 
    {
      return values[(int)i_x1];
    }
    else
    {
      double alpha=(i_x2-x)/(double)(i_x2-i_x1);
      double beta =1-alpha;
      return alpha*values[i_x1] + beta*values[i_x2];
    }
  }

  //setValue (x and y in range [0,1])
  void setValue(double x1, double y1, double x2, double y2)
  {
    if (x2<x1) 
    {
      std::swap(x1,x2);
      std::swap(y1,y2);
    }

    int N=(int)size();
    x1=Utils::clamp(x1,0.0,1.0); y1=Utils::clamp(y1,0.0,1.0); int i_x1=Utils::clamp((int)(round(x1*(N-1))),0,N-1);
    x2=Utils::clamp(x2,0.0,1.0); y2=Utils::clamp(y2,0.0,1.0); int i_x2=Utils::clamp((int)(round(x2*(N-1))),0,N-1);

    this->values[i_x1] = y1;
    this->values[i_x2] = y2;

    //interpolate
    for (int K = i_x1 + 1; K < i_x2; K++)
    {
      float beta  = (K - i_x1) / (float)(i_x2 - i_x1);
      float alpha = (1 - beta);
      values[K] = alpha*values[i_x1] + beta*values[i_x2];
    }
  }

  //setValue (x and y in range [0,1])
  void setValue(double x, double y) {
    setValue(x, y, x, y);
  }

public:

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) ;

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream) ;


};

////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API TransferFunction : public Model
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunction)

  typedef SingleTransferFunction Function;
  typedef ComputeRange InputNormalization;

  //constructor
  TransferFunction(String default_name = "") {
    if (!default_name.empty())
      setDefault(default_name);
  }

  //destructor
  virtual ~TransferFunction() {
  }

  //getInputNormalization
  InputNormalization getInputNormalization() const {
    return input.normalization;
  }

  //setInputNormalization
  void setInputNormalization(InputNormalization value);

  //getAttenuation
  double getAttenuation() const {
    return attenuation;
  }

  //setAttenuation
  void setAttenuation(double value);

  //getOutputDType
  DType getOutputDType() const {
    return output.dtype;
  }

  //setOutputDType
  void setOutputDType(DType value);

  //getOutputRange
  const Range& getOutputRange() const {
    return output.range;
  }

  //setOutputRange
  void setOutputRange(Range value);

  //size
  int size() const {
    return functions.empty() ? 0 : functions[0]->size();
  }

  //resize
  void resize(int value);

  //getFunctions
  const std::vector< SharedPtr<Function> >& getFunctions() const {
    return functions;
  }

  //getNumberOfFunctions
  inline int getNumberOfFunctions() const {
    return (int)functions.size();
  }

  //setNumberOfFunctions
  void setNumberOfFunctions(int value);

  //getFunction
  inline SharedPtr<Function> getFunction(int index) const {
    return functions[index];
  }

  //addFunction
  void addFunction(SharedPtr<Function> fn)
  {
    if (!functions.empty() && functions.back()->size() != fn->size())
      ThrowException("wrong function size");

    beginUpdate();
    functions.push_back(fn);
    endUpdate();
  }

  //getDefaults
  static std::vector<String> getDefaults();

  //getDefaultName
  String getDefaultName() const {
    return default_name;
  }

  //setDefault
  bool setDefault(String default_name);

  //setNotDefault
  void setNotDefault();

  //getInterpolationTypes
  static std::vector<String> getInterpolationTypes()
  {
    std::vector<String> ret;
    ret.push_back("Default");
    ret.push_back("Flat");
    ret.push_back("Inverted");
    return ret;
  }

  //getInterpolationName
  String getInterpolationName() const {
    return interpolation.toString();
  }

  //setInterpolation
  void setInterpolation(String name)
  {
    beginUpdate();
    interpolation.set(name); 
    if (!default_name.empty()) 
      setDefault(default_name);
    endUpdate();
  }

  //setFromArray
  bool setFromArray(Array src, String default_name);

  //convertToArray
  Array convertToArray() const;

  //importTransferFunction
  bool importTransferFunction(String url);

  //exportTransferFunction
  bool exportTransferFunction(String filename);

  //applyToArray
  Array applyToArray(Array src, Aborted aborted = Aborted());

  //copyFrom
  static void copy(TransferFunction& dst, const TransferFunction& src);

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;
  
  //writeToSceneObjectStream
  virtual void writeToSceneObjectStream(ObjectStream& ostream) override;
  
  //readFromObjectStream
  virtual void readFromSceneObjectStream(ObjectStream& istream) override;

private:

  InterpolationMode  interpolation;

  String default_name;

  //see https://github.com/sci-visus/visus-issues/issues/260
  double attenuation = 0.0;  

  struct
  {
    InputNormalization normalization;
  }
  input;

  struct
  {
    //what is the output (this must be atomic)
    DType dtype = DTypes::UINT8; 

     //how to map the range [0,1] to some user range
    Range range = Range(0, 255,1);
  }
  output;
  
  std::vector< SharedPtr<Function> > functions;

  //guessFunctionName
  String guessFunctionName();

  //guessFunctionColor
  Color guessFunctionColor();

};

typedef TransferFunction Palette;

} //namespace Visus

#endif //VISUS_TRANSFER_FUNCTION_H

