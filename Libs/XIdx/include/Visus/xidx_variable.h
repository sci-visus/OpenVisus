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

#ifndef XIDX_VARIABLE_H_
#define XIDX_VARIABLE_H_

#include <algorithm>

namespace Visus{

//////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API CenterType
{
public:

  enum Value {
    NODE_CENTER = 0,
    CELL_CENTER = 1,
    GRID_CENTER = 2,
    FACE_CENTER = 3,
    EDGE_CENTER = 4,
    END_ENUM
  };

  Value value;

  //constructor
  CenterType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static CenterType fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      CenterType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return CenterType();
  }

  //toString
  String toString() const {
    switch (value)
    {
    case NODE_CENTER:   return "Node";
    case CELL_CENTER:   return "Cell";
    case GRID_CENTER:   return "Grid";
    case FACE_CENTER:   return "Face";
    case EDGE_CENTER:   return "Edge";
    default:            return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return value == other;
  }

};


//////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API Variable : public XIdxElement
{
public:

  VISUS_XIDX_CLASS(Variable)

  CenterType               center_type;

  //down nodes
  std::vector<Attribute*> attributes;
  std::vector<DataItem*>  data_items;

  //constructor
  Variable(String name_="") : XIdxElement(name_) {
  }

  //constructor
  virtual ~Variable() {
    for (auto it : attributes)
      delete it;
    for (auto it : data_items)
      delete it;
  }
  
  //setValues
  virtual void setValues(std::vector<double> values,int stride=1)
  {
    ensureDataItem();
    this->data_items[0]->setValues(values,stride);
  }
  
  //getValues
  std::vector<double> getValues(int axis = 0) const{
    if(data_items.size() > axis)
      return data_items[axis]->values;
    else
      PrintInfo("Axis",axis,"does not exist");
    return std::vector<double>();
  }
  
  //getVolume
  virtual size_t getVolume() const{
    size_t total = 1;
    for(auto& item: this->data_items)
        total *= item->getVolume();
    return total;
  }
  
  //addAttribute
  virtual void addAttribute(Attribute* VISUS_DISOWN(value)){
    addEdge(this, value);
    this-> attributes.push_back(value);
  }

  //addAttribute
  void addAttribute(String name_, String value_){
    addAttribute(new Attribute(name_, value_));
  }

  //addDataItem
  virtual void addDataItem(DataItem* VISUS_DISOWN(value)){
    addEdge(this, value);
    this->data_items.push_back(value);
  }

  //addValue
  void addValue(double v)
  {
    if(data_items.size()==0)
    {
      DataItem* d=new DataItem();
      d->format_type=FormatType::XML_FORMAT;
      data_items.push_back(d);
    }
    this->data_items[0]->addValue(v);
  }

public:
  
  //write
  virtual void write(Archive& ar) const override
  {
    XIdxElement::write(ar);

    ar.write("Center", center_type.toString());

    for (auto child : data_items)
      writeChild<DataItem>(ar, "DataItem", child);

    for (auto child : attributes)
      writeChild<Attribute>(ar, "Attribute", child);
  }

  //read
  virtual void read(Archive& ar) override
  {
    XIdxElement::read(ar);

    this->center_type = CenterType::fromString(ar.readString("Center"));

    for (auto child : readChilds<DataItem>(ar, "DataItem"))
      addDataItem(child);

    for (auto child : readChilds<Attribute>(ar, "Attribute"))
      addAttribute(child);
  }

private:

  //ensureDataItem
  void ensureDataItem() {
    if (data_items.empty())
      addDataItem(new DataItem());
  }

};

} //namespace

#endif 
