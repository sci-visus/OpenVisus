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

#ifndef XIDX_DOMAIN_H_
#define XIDX_DOMAIN_H_

namespace Visus{

//////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API DomainType
{
public:

  enum Value
  {
    HYPER_SLAB_DOMAIN_TYPE = 0,
    LIST_DOMAIN_TYPE = 1,
    MULTIAXIS_DOMAIN_TYPE = 2,
    SPATIAL_DOMAIN_TYPE = 3,
    RANGE_DOMAIN_TYPE = 4,
    END_ENUM
  };

  Value value;

  //constructor
  DomainType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static DomainType fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      DomainType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return DomainType();
  }

  //toString
  String toString() const {
    switch (value)
    {
    case HYPER_SLAB_DOMAIN_TYPE:         return "HyperSlab";
    case LIST_DOMAIN_TYPE:               return "List";
    case MULTIAXIS_DOMAIN_TYPE:          return "MultiAxisDomain";
    case SPATIAL_DOMAIN_TYPE:            return "Spatial";
    case RANGE_DOMAIN_TYPE:              return "Range";
    default:                             return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return value == other;
  }

};

//////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API Domain : public XIdxElement
{
public:

  VISUS_XIDX_CLASS(Domain)

  typedef std::vector<double> LinearizedIndexSpace;

  //node info
  DomainType              type;

  //down nodes
  std::vector<Attribute*> attributes;
  std::vector<DataItem*>  data_items;
  
  //constructor
  Domain(String name_,DomainType type_=DomainType()) 
    : XIdxElement(name_),type(type_) {
  }

  //destructor
  virtual ~Domain() 
  {
    for (auto it : attributes)
      delete it;

    for (auto it : data_items)
      delete it;
  }
  
  //ensureDataItem
  void ensureDataItem() {
    if (data_items.empty()) 
      addDataItem(new DataItem());
  }

  //createDomain
  static VISUS_NEWOBJECT(Domain*) createDomain(DomainType type);

  //addDataItem
  void addDataItem(DataItem* VISUS_DISOWN(value)){
    addEdge(this, value);
    this->data_items.push_back(value);
  }
  
  //addAttribute
  void addAttribute(Attribute* VISUS_DISOWN(value)) {
    addEdge(this, value);
    this->attributes.push_back(value);
  }

  //addAttribute
  void addAttribute(String name_, String value_){
    addAttribute(new Attribute(name_, value_));
  }

  //getVolume
  virtual size_t getVolume() const{
    size_t total = 1;
    for(auto item: this->data_items)
      for(int i=0; i < item->dimensions.size(); i++)
        total *= item->dimensions[i];
    return total;
  }
  
  //getLinearizedIndexSpace
  virtual LinearizedIndexSpace getLinearizedIndexSpace() = 0;
  
public:

  //write
  virtual void write(Archive& ar) const override
  {
    XIdxElement::write(ar);

    ar.write("Type", type.toString());

    for (auto child : data_items)
      writeChild<DataItem>(ar, "DataItem", child);

    for (auto child : attributes)
      writeChild<Attribute>(ar, "Attribute", child);
  }

  //read
  virtual void read(Archive& ar) override
  {
    XIdxElement::read(ar);

    this->type = DomainType::fromString(ar.readString("Type"));

    // have to remove the default data_item created by the constructor
    // TODO improve this ensureDataItem() mechanism
    data_items.clear();

    for (auto child : readChilds<DataItem>(ar, "DataItem"))
      addDataItem(child);

    for (auto child : readChilds<Attribute>(ar, "Attribute"))
      addAttribute(child);
  }

};

} //namespace

#endif //XIDX_DOMAIN_H_

