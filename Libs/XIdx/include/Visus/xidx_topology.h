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

#ifndef XIDX_TOPOLOGY_H_
#define XIDX_TOPOLOGY_H_

namespace Visus{

  ///////////////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API TopologyType
{
public:

  enum Value
  {
    NO_TOPOLOGY_TYPE = 0,
    RECT_2D_MESH_TOPOLOGY_TYPE = 1,
    CORECT_2D_MESH_TOPOLOGY_TYPE = 2,
    RECT_3D_MESH_TOPOLOGY_TYPE = 3,
    CORECT_3D_MESH_TOPOLOGY_TYPE = 4,
    DIM_1D_TOPOLOGY_TYPE = 5,
    END_ENUM
  };

  Value value;

  //constructor
  TopologyType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static TopologyType fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      TopologyType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return TopologyType();
  }

  //toString
  String toString() const
  {
    switch (value)
    {
    case NO_TOPOLOGY_TYPE:              return "NoTopologyType";
    case RECT_2D_MESH_TOPOLOGY_TYPE:    return "2DRectMesh";
    case RECT_3D_MESH_TOPOLOGY_TYPE:    return "3DRectMesh";
    case CORECT_2D_MESH_TOPOLOGY_TYPE:  return "2DCoRectMesh";
    case CORECT_3D_MESH_TOPOLOGY_TYPE:  return "3DCoRectMesh";
    case DIM_1D_TOPOLOGY_TYPE:          return "1D";
    default:                            return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return this->value == other;
  }

};

///////////////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API Topology : public XIdxElement
{
public:

  VISUS_XIDX_CLASS(Topology)

  //node info
  TopologyType             type;
  std::vector<int>          dimensions;

  //down nodes
  std::vector<Attribute*>  attributes;
  std::vector<DataItem*>   data_items;

  //construtor
  Topology(String name_="") : XIdxElement(name_){
  }

  //constructor
  Topology(TopologyType type_, int n_dims, uint32_t *dims){
    for(int i=0; i< n_dims; i++)
      dimensions.push_back(dims[i]);
    type = type_;
  }

  //destructor
  virtual ~Topology() 
  {
    for (auto it : attributes)
      delete it;

    for (auto it : data_items)
      delete it;
  }

  //addAttribute
  void addAttribute(Attribute* VISUS_DISOWN(value)) {
    addEdge(this, value);
    attributes.push_back(value);
  }

  //addDataItem
  void addDataItem(DataItem* VISUS_DISOWN(value)) {
    addEdge(this, value);
    data_items.push_back(value);
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    XIdxElement::writeToObjectStream(ostream);

    ostream.writeInline("Type", type.toString());
    ostream.writeInline("Dimensions", StringUtils::join(dimensions));

    for (auto child : this->attributes)
      writeChild<Attribute>(ostream, "Attribute", child);

    for (auto child : this->data_items)
      writeChild<DataItem>(ostream, "DataItem", child);
  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override
  {
    XIdxElement::readFromObjectStream(istream);

    this->type = TopologyType::fromString(istream.readInline("Type"));

    for (auto dim : StringUtils::split(istream.readInline("Dimensions")))
      this->dimensions.push_back(cint(dim));

    while (auto child=readChild<Attribute>(istream,"Attribute"))
      addAttribute(child);

    while (auto child = readChild<DataItem>(istream, "DataItem"))
      addDataItem(child);
  }
  
};

} //namespace

#endif
