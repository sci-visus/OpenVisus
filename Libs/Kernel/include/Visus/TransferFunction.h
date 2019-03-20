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
    Default,
    Flat,
    Inverted
  };

  //InterpolationMode
  InterpolationMode()  {
  }

  //toString
  String toString() const
  {
    switch (type)
    {
      case Flat    : return "Flat";
      case Inverted: return "Inverted";
      case Default :
      default      : return "Default";
    }
  }

  //set
  void set(const String &name)
  {
    if      (name=="Default" ) type=Default;
    else if (name=="Flat"    ) type=Flat;
    else if (name=="Inverted") type=Inverted;
  }

  //get
  Type get() { 
    return type; 
  }

  //getValues
  static std::vector<String> getValues(){
    return { "Default" ,"Flat","Inverted" };
  }

private:

  Type type=Default;

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
  Color getColor(double alpha,InterpolationMode::Type type=InterpolationMode::Default);

  //convertToArray
  void convertToArray(Array& dst,int nsamples,InterpolationMode::Type type=InterpolationMode::Default);

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream);

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream);

};


////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API TransferFunction : public Model
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunction)

  //________________________________________________________________
  class VISUS_KERNEL_API Single
  {
  public:

    VISUS_CLASS(Single)

    String              name;
    Color               color;
    std::vector<double> values;

    //constructor (identity function)
    Single(int nsamples = 256) 
    {
      for (int I = 0; I<nsamples; I++)
        values.push_back(I / (double)(nsamples - 1));
    }

    //constructor (identity function)
    Single(String name , Color color = Colors::Black, int nsamples = 256) : Single(nsamples)
    {
      this->name = name;
      this->color = color;
    }

    //destructor
    virtual ~Single() {
    }

    //size
    inline int size() const {
      return (int)values.size();
    }

    //resize
    void resize(int value)
    {
      if (value == this->size()) return;
      std::vector<double> values(value);
      for (int I = 0; I<value; I++) 
        values[I] = this->getValue(I / (double)(value - 1));
      this->values = values;
    }

    //getValue (x must be in range [0,1])
    double getValue(double x) const
    {
      int N = size();
      if (!N) {
        VisusAssert(false);
        return 0;
      }

      x = Utils::clamp(x*(N - 1), 0.0, N - 1.0);

      int i_x1 = Utils::clamp((int)std::floor(x), 0, N - 1);
      int i_x2 = Utils::clamp((int)std::ceil(x), 0, N - 1);

      if (i_x1 == i_x2)
      {
        return values[(int)i_x1];
      }
      else
      {
        double alpha = (i_x2 - x) / (double)(i_x2 - i_x1);
        double beta = 1 - alpha;
        return alpha * values[i_x1] + beta * values[i_x2];
      }
    }

    //setValue (x and y in range [0,1])
    void setValue(double x1, double y1, double x2, double y2)
    {
      if (x2<x1)  {std::swap(x1, x2); std::swap(y1, y2);}
      int N = (int)size();
      x1 = Utils::clamp(x1, 0.0, 1.0); y1 = Utils::clamp(y1, 0.0, 1.0); int i_x1 = Utils::clamp((int)(round(x1*(N - 1))), 0, N - 1);
      x2 = Utils::clamp(x2, 0.0, 1.0); y2 = Utils::clamp(y2, 0.0, 1.0); int i_x2 = Utils::clamp((int)(round(x2*(N - 1))), 0, N - 1);
      this->values[i_x1] = y1;
      this->values[i_x2] = y2;
      //interpolate
      for (int K = i_x1 + 1; K < i_x2; K++)
      {
        float beta = (K - i_x1) / (float)(i_x2 - i_x1);
        float alpha = (1 - beta);
        values[K] = alpha * values[i_x1] + beta * values[i_x2];
      }
    }

    //setValue (x and y in range [0,1])
    void setValue(double x, double y) {
      setValue(x, y, x, y);
    }

  public:

    //writeToObjectStream
    void writeToObjectStream(ObjectStream& ostream);

    //readFromObjectStream
    void readFromObjectStream(ObjectStream& istream);

  };

  String default_name;

  //see https://github.com/sci-visus/visus-issues/issues/260
  double attenuation = 0.0;

  //interpolation
  InterpolationMode interpolation;

  //input_range
  ComputeRange input_range;

  //what is the output (this must be atomic)
  DType output_dtype = DTypes::UINT8;

  //how to map the range [0,1] to some user range
  Range output_range = Range(0, 255, 1);

  //functions
  std::vector< SharedPtr<Single> > functions;

  //constructor
  TransferFunction(String default_name = "") {
    if (!default_name.empty())
      setDefault(default_name);
  }

  //destructor
  virtual ~TransferFunction() {
  }

  //getTypeName
  virtual String getTypeName() const  override {
    return "TransferFunction";
  }

  //size
  int size() const {
    return functions.empty() ? 0 : functions[0]->size();
  }

  //addFunction
  SharedPtr<Single> addFunction(String name, Color color, int nsamples = 256);

  //setNumberOfFunctions
  void setNumberOfFunctions(int value, std::vector<String> names = { "R","G","B","A" }, std::vector<Color> colors = { Colors::Red,Colors::Green,Colors::Blue,Colors::Gray });

  //getDefaults
  static std::vector<String> getDefaults();

  //setDefault
  bool setDefault(String default_name);

  //setNotDefault
  void setNotDefault();

  //copyFrom
  static void copy(TransferFunction& dst, const TransferFunction& src);

public:

  //applyToArray
  Array applyToArray(Array src, Aborted aborted = Aborted());

public:

  //setFromArray
  bool setFromArray(Array src, String default_name, std::vector<String> names = {"R","G","B","A"}, std::vector<Color> colors = { Colors::Red,Colors::Green,Colors::Blue,Colors::Gray });

  //convertToArray
  Array convertToArray() const;

  //importTransferFunction
  bool importTransferFunction(String url);

  //exportTransferFunction
  bool exportTransferFunction(String filename);

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;
  
};

typedef TransferFunction Palette;

} //namespace Visus

#endif //VISUS_TRANSFER_FUNCTION_H

