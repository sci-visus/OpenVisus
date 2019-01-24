/*
 * Copyright (c) 2017 University of Utah
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XIDX_GEOMETRY_H_
#define XIDX_GEOMETRY_H_

namespace Visus{

//////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API GeometryType
{

public:

  enum Value
  {
    XYZ_GEOMETRY_TYPE = 0,
    XY_GEOMETRY_TYPE = 1,
    X_Y_Z_GEOMETRY_TYPE = 2,
    VXVYVZ_GEOMETRY_TYPE = 3,
    ORIGIN_DXDYDZ_GEOMETRY_TYPE = 4,
    ORIGIN_DXDY_GEOMETRY_TYPE = 5,
    RECT_GEOMETRY_TYPE = 6,
    END_ENUM
  };

  Value value;

  //constructor
  GeometryType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static GeometryType fromString(String value) {

    for (int I = 0; I < END_ENUM; I++) {
      GeometryType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }

    ThrowException("invalid enum value");
    return GeometryType();
  }

  //toString
  String toString() const
  {
    switch (value)
    {
    case XYZ_GEOMETRY_TYPE:               return "XYZ";
    case XY_GEOMETRY_TYPE:                return "XY";
    case X_Y_Z_GEOMETRY_TYPE:             return "X_Y_Z";
    case VXVYVZ_GEOMETRY_TYPE:            return "VxVyVz";
    case ORIGIN_DXDYDZ_GEOMETRY_TYPE:     return "Origin_DxDyDz";
    case ORIGIN_DXDY_GEOMETRY_TYPE:       return "Origin_DxDy";
    case RECT_GEOMETRY_TYPE:              return "Rect";
    default:                              return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return value == other;
  }

};



//////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API Geometry : public XIdxElement
{
public:

  VISUS_CLASS(Geometry)
  
  GeometryType                             type;
  std::vector< std::shared_ptr<DataItem> > data_items;
  
  //constructor
  Geometry(String name_="") : XIdxElement(name_){
  }

  //destructor
  virtual ~Geometry() {
  }

  //addDataItem
  void addDataItem(SharedPtr<DataItem> item) {
    item->setParent(this);
    this->data_items.push_back(item);
  }

  //getVolume
  size_t getVolume() const{
    size_t total = 1;
    for(auto item: this->data_items){
      for(auto dim : item->dimensions)
        total *= dim;
    }
    return total;
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    XIdxElement::writeToObjectStream(ostream);

    ostream.writeInline("Type", type.toString());

    for (auto child : this->data_items)
      writeChild<DataItem>(ostream,"DataItem",child);
  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override
  {
    XIdxElement::readFromObjectStream(istream);

    this->type = GeometryType::fromString(istream.readInline("Type"));

    while (auto child = readChild<DataItem>(istream,"DataItem"))
      addDataItem(child);
  }

};

} //namespace

#endif //XIDX_GEOMETRY_H_

