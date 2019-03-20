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

  VISUS_XIDX_CLASS(Geometry)
  
  GeometryType type;

  //down nodes
  std::vector<DataItem*> data_items;
  
  //constructor
  Geometry(String name_="") : XIdxElement(name_){
  }

  //constructor
  Geometry(GeometryType type_){
    type = type_;
  }

  //constructor
  Geometry(GeometryType type_, DataItem* VISUS_DISOWN(item)){
    type = type_;
    addDataItem(item);
  }

  //constructor
  Geometry(GeometryType type_, int n_dims, const double* ox_oy_oz, const double* dx_dy_dz=NULL) 
  {
    this->type = type_;

    DataItem* item_o=new DataItem();
    item_o->format_type = FormatType::XML_FORMAT;
    item_o->dtype = DTypes::FLOAT32;
    item_o->endian_type = Endianess::LITTLE_ENDIANESS;
    item_o->dimensions.push_back(n_dims);
    addDataItem(item_o);

    if(type == GeometryType::RECT_GEOMETRY_TYPE)
    {
      n_dims *= 2; // two points per dimension
      for(int i=0; i< n_dims; i++)
        item_o->text += (i ? " " : "") + cstring(ox_oy_oz[i])+" ";
    }
    else
    {
      DataItem* item_d=new DataItem();
      item_d->format_type = FormatType::XML_FORMAT;
      item_d->dtype = DTypes::FLOAT32;
      item_d->endian_type = Endianess::LITTLE_ENDIANESS;
      item_d->dimensions.push_back(n_dims);
      addDataItem(item_d);

      for(int i=0; i< n_dims; i++)
      {
        item_o->text += (i ? " " : "") + cstring(ox_oy_oz[i]);
        item_d->text += (i ? " " : "") + cstring(dx_dy_dz[i]);
      }
    }
  }

  //destructor
  virtual ~Geometry() {
    for (auto it : data_items)
      delete it;
  }

  //addDataItem
  void addDataItem(DataItem* VISUS_DISOWN(value)) {
    addEdge(this,value);
    this->data_items.push_back(value);
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

