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

#ifndef __VISUS_COLOR_H
#define __VISUS_COLOR_H

#include <Visus/Kernel.h>
#include <Visus/Utils.h>

#include <array>
#include <iomanip>

namespace Visus {
 
////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Color
{
public:

  VISUS_CLASS(Color)

  enum ColorModel
  {
    RGBType,
    HSBType,
    HLSType,
    CieLabType
  };

  //constructor
  Color() : color_model(RGBType) ,v({{0,0,0,1}}) {
  }
  
  //constructor
  Color(Float32 C0,Float32 C1,Float32 C2,Float32 Alpha=1.0f,ColorModel type_=RGBType) : color_model(type_),v({{C0,C1,C2,Alpha}})
  {
    if (color_model!=CieLabType) 
      clampToRange(0.0f,1.0f);
  }

  //constructor
  Color(int C0,int C1,int C2,int Alpha=255) : color_model(RGBType),v({{C0/255.0f,C1/255.0f,C2/255.0f,Alpha/255.0f}}) {
    clampToRange(0.0f,1.0f);
  }
  
  //destructor
  virtual ~Color()
  {}

  //clampToUnitRange
  void clampToRange(Float32 a,Float32 b)
  {
    this->v[0]=Utils::clamp(this->v[0],a,b);
    this->v[1]=Utils::clamp(this->v[1],a,b);
    this->v[2]=Utils::clamp(this->v[2],a,b);
    this->v[3]=Utils::clamp(this->v[3],a,b);
  }

  //parseFromString
  static Color parseFromString(String value) 
  {
    std::istringstream parser(value);
    int R,G,B,A;parser>>R>>G>>B>>A;
    return Color(R,G,B,A);
  }

  //createFromUint32 (format 0xRRGGBBAA)
  static Color createFromUint32(Uint32 value) 
  {
    int A=value & 0xff; value>>=8; 
    int B=value & 0xff; value>>=8;
    int G=value & 0xff; value>>=8;
    int R=value & 0xff; value>>=8;
    return Color(R,G,B,A);
  }

  //random color
  static Color random(Float32 Alpha=1.0)
  {
    Float32 C0=(Float32)Utils::getRandDouble(0,1);
    Float32 C1=(Float32)Utils::getRandDouble(0,1);
    Float32 C2=(Float32)Utils::getRandDouble(0,1);
    return Color(C0,C1,C2,Alpha,RGBType);
  }

  //getColorModel
  ColorModel getColorModel() const{
    return color_model;
  }

  //setColorModel
  void setColorModel(ColorModel value){
    this->color_model = value;
  }

  //get
  Float32 get(int index) const
  {VisusAssert(index>=0 && index<4);return v[index];}

  //set
  void set(int index,Float32 value)
  {
    VisusAssert(index>=0 && index<4);
    v[index]=color_model==CieLabType? value : Utils::clamp(value,0.0f,1.0f);
  }

  //operator[]
  const Float32& operator[](int index) const
  {
    VisusAssert(index>=0 && index<4);
    return v[index];
  }

  //operator[]
  Float32& operator[](int index) 
  {
    VisusAssert(index>=0 && index<4);
    return v[index];
  }
  
  //operator=
  Color& operator=(const Color& other)
  {
    this->color_model=other.color_model;
    this->v[0]=other.v[0];
    this->v[1]=other.v[1];
    this->v[2]=other.v[2];
    this->v[3]=other.v[3];
    return *this;
  }

  //distance (ignores alpha)
  static Float32 distance(const Color& c1, const Color& c2)
  {
    Float32 d0 =c1[0] -c2[0];
    Float32 d1 =c1[1]- c2[1];
    Float32 d2 =c1[2]- c2[2];
    return sqrt((d0*d0)+(d1*d1)+(d2*d2));
  }

  //operator==
  bool operator==(const Color& other) const
  {
    return 
      color_model==other.color_model && 
      v[0]==other.v[0] && 
      v[1]==other.v[1] && 
      v[2]==other.v[2] && 
      v[3]==other.v[3];
  }

  //operator!=
  bool operator!=(const Color& other) const{
    return !(*this == other);
  }

  //a*f
  Color operator*(Float32 s) const{
    return Color(s*v[0], s*v[1], s*v[2], s*v[3], color_model);
  }

  //withAlpha
  Color withAlpha(Float32 alpha) const{
    return Color(v[0], v[1], v[2], alpha, color_model);
  }

  //convertToOtherModel
  Color convertToOtherModel(ColorModel color_model) const;

  //convertToXXX
  Color toRGB   () const {return convertToOtherModel(RGBType);}
  Color toHSB   () const {return convertToOtherModel(HSBType);}
  Color toHLS   () const {return convertToOtherModel(HLSType);}
  Color toCieLab() const {return convertToOtherModel(CieLabType);}

public:

  Float32     getRed       () const          {VisusAssert(color_model==RGBType                        );return get(0);}
  Float32     getGreen     () const          {VisusAssert(color_model==RGBType                        );return get(1);}
  Float32     getBlue      () const          {VisusAssert(color_model==RGBType                        );return get(2);}
  Float32     getHue       () const          {VisusAssert(color_model==HSBType || color_model==HLSType);return get(                   0);}
  Float32     getSaturation() const          {VisusAssert(color_model==HSBType || color_model==HLSType);return get(color_model==HSBType? 1 : 2);}
  Float32     getLightness () const          {VisusAssert(color_model==HLSType                        );return get(                   1);}
  Float32     getBrightness() const          {VisusAssert(color_model==HSBType                        );return get(                   2);}
  Float32     getAlpha     () const          {                                                          return get(                   3);}

  void        setRed        (Float32 value)  {VisusAssert(color_model==RGBType                        );set(                   0,value);}
  void        setGreen      (Float32 value)  {VisusAssert(color_model==RGBType                        );set(                   1,value);}
  void        setBlue       (Float32 value)  {VisusAssert(color_model==RGBType                        );set(                   2,value);}
  void        setHue        (Float32 value)  {VisusAssert(color_model==HSBType || color_model==HLSType);set(                   0,value);}
  void        setSaturation (Float32 value)  {VisusAssert(color_model==HSBType || color_model==HLSType);set(color_model==HSBType? 1 : 2,value);}
  void        setLightness  (Float32 value)  {VisusAssert(color_model==HLSType                        );set(                   1,value);}
  void        setBrightness (Float32 value)  {VisusAssert(color_model==HSBType                        );set(                   2,value);}
  void        setAlpha      (Float32 value)  {                                                          set(                   3,value);}
  
  //interpolate
  static Color interpolate(Float32 alpha, Color c1, Float32 beta, Color c2)
  {
    c2=c2.convertToOtherModel(c1.color_model);
    VisusAssert(c1.color_model==c2.color_model);
    return Color(alpha*c1[0] + beta*c2[0],
                 alpha*c1[1] + beta*c2[1],
                 alpha*c1[2] + beta*c2[2],
                 alpha*c1[3] + beta*c2[3],
                 c1.color_model);
  }

public:

  //toString
  String toString() const 
  {
    Color rgb=toRGB();
    std::ostringstream o; 
    o<<(int)(255.0*rgb.get(0))<<" ";
    o<<(int)(255.0*rgb.get(1))<<" ";
    o<<(int)(255.0*rgb.get(2))<<" ";
    o<<(int)(255.0*rgb.get(3)); 
    return o.str();
  }

  //toHexString (0xRRGGBBAA)
  String toHexString() const
  {
    Color rgb=toRGB();
    std::stringstream stream;
    stream <<"0x"<<std::hex
    <<std::setfill('0')<<std::setw(2)<<(int)(255.0*rgb.get(0))
    <<std::setfill('0')<<std::setw(2)<<(int)(255.0*rgb.get(1))
    <<std::setfill('0')<<std::setw(2)<<(int)(255.0*rgb.get(2))
    <<std::setfill('0')<<std::setw(2)<<(int)(255.0*rgb.get(3));
    return stream.str();
  }
  
  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) 
  {
    Color rgb=toRGB();
    ostream.writeInline("r",cstring((int)(255.0*rgb.get(0))));
    ostream.writeInline("g",cstring((int)(255.0*rgb.get(1))));
    ostream.writeInline("b",cstring((int)(255.0*rgb.get(2))));
    ostream.writeInline("a",cstring((int)(255.0*rgb.get(3))));
  }
  
  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream) 
  {
    int R=cint(istream.readInline("r"));
    int G=cint(istream.readInline("g"));
    int B=cint(istream.readInline("b"));
    int A=cint(istream.readInline("a"));
    (*this)=Color(R,G,B,A);
  }

private:

  ColorModel color_model;
  std::array<Float32,4> v;
  
};
  
#if !SWIG
inline Color operator*(Float32 s,const Color &c) {
  return c*s;
}
#endif

  
////////////////////////////////////////////////////////
//see http://kb.iu.edu/data/aetf.html
////////////////////////////////////////////////////////
#if !SWIG
class VISUS_KERNEL_API Colors
{
public:
  static const Color  Transparent;
  static const Color  AliceBlue;
  static const Color  AntiqueWhite;
  static const Color  Aqua;
  static const Color  Aquamarine;
  static const Color  Azure;
  static const Color  Beige;
  static const Color  Bisque;
  static const Color  Black;
  static const Color  BlanchedAlmond;
  static const Color  Blue;
  static const Color  BlueViolet;
  static const Color  Brown;
  static const Color  Burlywood;
  static const Color  CadetBlue;
  static const Color  Chartreuse;
  static const Color  Chocolate;
  static const Color  Coral;
  static const Color  CornflowerBlue;
  static const Color  Cornsilk;
  static const Color  Cyan;
  static const Color  DarkBlue;
  static const Color  DarkCyan;
  static const Color  DarkGoldenrod;
  static const Color  DarkGray;
  static const Color  DarkGreen;
  static const Color  DarkKhaki;
  static const Color  DarkMagenta;
  static const Color  DarkOliveGreen;
  static const Color  DarkOrange;
  static const Color  DarkOrchid;
  static const Color  DarkRed;
  static const Color  DarkSalmon;
  static const Color  DarkSeaGreen;
  static const Color  DarkSlateBlue;
  static const Color  DarkSlateGray;
  static const Color  DarkTurquoise;
  static const Color  DarkViolet;
  static const Color  DeepPink;
  static const Color  DeepSkyBlue;
  static const Color  DimGray;
  static const Color  DodgerBlue;
  static const Color  Firebrick;
  static const Color  FloralWhite;
  static const Color  ForestGreen;
  static const Color  Fuschia;
  static const Color  Gainsboro;
  static const Color  GhostWhite;
  static const Color  Gold;
  static const Color  Goldenrod;
  static const Color  Gray;
  static const Color  Green;
  static const Color  GreenYellow;
  static const Color  Honeydew;
  static const Color  HotPink;
  static const Color  IndianRed;
  static const Color  Ivory;
  static const Color  Khaki;
  static const Color  Lavender;
  static const Color  LavenderBlush;
  static const Color  LawnGreen;
  static const Color  LemonChiffon;
  static const Color  LightBlue;
  static const Color  LightCoral;
  static const Color  LightCyan;
  static const Color  LightGoldenrod;
  static const Color  LightGoldenrodYellow;
  static const Color  LightGray;
  static const Color  LightGreen;
  static const Color  LightPink;
  static const Color  LightSalmon;
  static const Color  LightSeaGreen;
  static const Color  LightSkyBlue;
  static const Color  LightSlateBlue;
  static const Color  LightSlateGray;
  static const Color  LightSteelBlue;
  static const Color  LightYellow;
  static const Color  Lime;
  static const Color  LimeGreen;
  static const Color  Linen;
  static const Color  Magenta;
  static const Color  Maroon;
  static const Color  MediumAquamarine;
  static const Color  MediumBlue;
  static const Color  MediumOrchid;
  static const Color  MediumPurple;
  static const Color  MediumSeaGreen;
  static const Color  MediumSlateBlue;
  static const Color  MediumSpringGreen;
  static const Color  MediumTurquoise;
  static const Color  MediumVioletRed;
  static const Color  MidnightBlue;
  static const Color  MintCream;
  static const Color  MistyRose;
  static const Color  Moccasin;
  static const Color  NavajoWhite;
  static const Color  Navy;
  static const Color  OldLace;
  static const Color  Olive;
  static const Color  OliveDrab;
  static const Color  Orange;
  static const Color  OrangeRed;
  static const Color  Orchid;
  static const Color  PaleGoldenrod;
  static const Color  PaleGreen;
  static const Color  PaleTurquoise;
  static const Color  PaleVioletRed;
  static const Color  PapayaWhip;
  static const Color  PeachPuff;
  static const Color  Peru;
  static const Color  Pink;
  static const Color  Plum;
  static const Color  PowderBlue;
  static const Color  Purple;
  static const Color  Red;
  static const Color  RosyBrown;
  static const Color  RoyalBlue;
  static const Color  SaddleBrown;
  static const Color  Salmon;
  static const Color  SandyBrown;
  static const Color  SeaGreen;
  static const Color  Seashell;
  static const Color  Sienna;
  static const Color  Silver;
  static const Color  SkyBlue;
  static const Color  SlateBlue;
  static const Color  SlateGray;
  static const Color  Snow;
  static const Color  SpringGreen;
  static const Color  SteelBlue;
  static const Color  Tan;
  static const Color  Teal;
  static const Color  Thistle;
  static const Color  Tomato;
  static const Color  Turquoise;
  static const Color  Violet;
  static const Color  VioletRed;
  static const Color  Wheat;
  static const Color  White;
  static const Color  WhiteSmoke;
  static const Color  Yellow;
  static const Color  YellowGreen;

private:

  Colors() = delete;
    
};
#endif
  
} //namespace Visus
  
#endif //__VISUS_COLOR_H
  
