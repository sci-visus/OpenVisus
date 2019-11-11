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

#ifndef XIDX_GROUP_H_
#define XIDX_GROUP_H_


namespace Visus{

  ///////////////////////////////////////////////////////////////////
class VISUS_XIDX_API GroupType
{
public:

  enum Value {
    SPATIAL_GROUP_TYPE = 0,
    TEMPORAL_GROUP_TYPE = 1,
    END_ENUM
  };

  Value value;

  //constructor
  GroupType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static GroupType fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      GroupType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return GroupType();
  }

  //cstring
  String toString() const
  {
    switch (value)
    {
    case SPATIAL_GROUP_TYPE:    return "Spatial";
    case TEMPORAL_GROUP_TYPE:   return "Temporal";
    default:                    return "[Unknown]";
    }
  }

};

///////////////////////////////////////////////////////////////////
class VISUS_XIDX_API VariabilityType
{
public:

  enum Value {
    STATIC_VARIABILITY_TYPE = 0,
    VARIABLE_VARIABILITY_TYPE = 1,
    END_ENUM
  };

  Value value;

  //constructor
  VariabilityType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static VariabilityType fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      VariabilityType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return VariabilityType();
  }

  //cstring
  String toString() const
  {
    switch (value)
    {
    case STATIC_VARIABILITY_TYPE:     return "Static";
    case VARIABLE_VARIABILITY_TYPE:   return "Variable";
    default:                          return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return this->value == other;
  }

};


///////////////////////////////////////////////////////////////////
class VISUS_XIDX_API Group : public XIdxElement 
{
public:

  VISUS_XIDX_CLASS(Group)

  // node info
  int                      domain_index = 0;
  String                   file_pattern;
  GroupType                group_type;
  VariabilityType          variability_type;

  //down nodes
  Domain*                  domain=nullptr;
  std::vector<Variable*>   variables;
  std::vector<DataSource*> data_sources;
  std::vector<Attribute*>  attributes;
  std::vector<Group*>      groups;

  //constructor
  Group(String name_=""){
    name= name_;
  }

  //constructor
  Group(String name_, GroupType type){
    name=name_;
    group_type=type;
  }

  //constructor
  Group(String name_, GroupType type, String file_pattern_){
    name=name_;
    group_type=type;
    file_pattern=file_pattern_;
    variability_type=VariabilityType::VARIABLE_VARIABILITY_TYPE;
  }

  //constructor
  Group(String name_, GroupType type, VariabilityType var_type){
    name=name_;
    group_type=type;
    variability_type=var_type;
  }

  //constructor
  Group(String name_, GroupType type, Domain* VISUS_DISOWN(domain)){
    name=name_;
    group_type=type;
    setDomain(domain);
  }

  //destructor
  virtual ~Group() 
  {
    setDomain(nullptr);

    for (auto it : variables)
      delete it;
    
    for (auto it : data_sources)
      delete it;

    for (auto it : attributes)
      delete it;

    for (auto it : groups)
      delete it;
  }

  //setDomain
  void setDomain(Domain* VISUS_DISOWN(value)) { 

    if (this->domain) {
      removeEdge(this, this->domain);
      delete this->domain;
    }

    this->domain = value;

    if (this->domain)
      addEdge(this, value);
  }

  //getDomain
  Domain* getDomain(){
    return domain;
  }

  //addVariable
  void addVariable(Variable* VISUS_DISOWN(value)) {
    addEdge(this, value);
    variables.push_back(value);
  }

  //getGroup
  Group* getGroup(int index){
    if (variability_type == VariabilityType::STATIC_VARIABILITY_TYPE)
      index = 0;
    return groups[index];
  }

  //addVariable
  Variable* addVariable(const char* name, DataItem* VISUS_DISOWN(data_item), Domain* VISUS_DISOWN(domain)){

    setDomain(domain);

    Variable* var=new Variable(name);
    addVariable(var);
    var->addDataItem(data_item);
    return var;
  }

  //addVariable
  Variable* addVariable(const char *name, DType dtype,
    const CenterType center = CenterType::CELL_CENTER,
    const Endianess endian = Endianess::LITTLE_ENDIANESS,
    const std::vector<int> dimensions = std::vector<int>())
  {
    Variable* var=new Variable(name);
    var->center_type = center;
    addVariable(var);

    DataItem* di=new DataItem(dtype);
    di->endian_type = endian;
    di->dimensions = dimensions.size()>0? dynamic_cast<SpatialDomain*>(domain)->topology->dimensions : dimensions; // Use same dimensions of topology
    di->format_type = FormatType::IDX_FORMAT;
    var->addDataItem(di);
    return var;
  }

  //addAttribute
  void addAttribute(Attribute* VISUS_DISOWN(value)) {
    addEdge(this, value);
    attributes.push_back(value);
  }

  //addDataSource
  void addDataSource(DataSource* VISUS_DISOWN(value)) {
    addEdge(this, value);
    data_sources.push_back(value);
  }

  //addGroup
  void addGroup(Group* VISUS_DISOWN(value))
  {
    if(value->variability_type == VariabilityType::VARIABLE_VARIABILITY_TYPE)
      value->domain_index = (int)groups.size();
    
    addEdge(this,value);
    groups.push_back(value);
  }

  //getXPathPrefix
  virtual String getXPathPrefix() override {
    return concatenate(getParent()? getParent()->getXPathPrefix() : "//Xidx","/Group","[@Name=\"", name,"\"]");
  }

public:

  //write
  virtual void write(Archive& ar) const override
  {
    XIdxElement::write(ar);

    ar.write("Name", name);
    ar.write("Type", group_type.toString());
    ar.write("VariabilityType", variability_type.toString());
    if (file_pattern.size())
      ar.write("FilePattern", file_pattern.c_str());

    if (variability_type.value != VariabilityType::STATIC_VARIABILITY_TYPE)
      ar.write("DomainIndex", Visus::cstring(domain_index));

    for (auto child : data_sources)
      writeChild<DataSource>(ar, "DataSource", child);

    writeChild<Domain>(ar, "Domain", domain);

    for (auto child : attributes)
      writeChild<Attribute>(ar, "Attribute", child);

    for (auto child : variables)
      writeChild<Variable>(ar, "Variable", child);

    for (auto child : groups)
    {
      if (file_pattern.empty())
      {
        writeChild<Group>(ar, "Group", child);
      }
      else
      {
        String filename = XIdxFormatString(file_pattern + "/meta.xidx", child->domain_index);

        ar.addChild(StringTree("xi:include")
          .write("href", filename)
          .write("xpointer", "xpointer(//Xidx/Group/Group)"));

        {
          StringTree stree(child->getTypeName());
          child->write(stree);
          auto content = stree.toString();
          Utils::saveTextDocument(filename, content);
        }
      }
    }
  };

  //read
  virtual void read(Archive& ar) override
  {
    XIdxElement::read(ar);

    this->group_type = GroupType::fromString(ar.readString("Type"));
    this->variability_type = VariabilityType::fromString(ar.readString("VariabilityType"));
    this->file_pattern = ar.readString("FilePattern");
    this->domain_index = cint(ar.readString("DomainIndex"));

    for (auto child : readChilds<DataSource>(ar, "DataSource"))
      addDataSource(child);

    for (auto child : readChilds<Attribute>(ar, "Attribute"))
      addAttribute(child);

    for (auto child : readChilds<Variable>(ar, "Variable"))
      addVariable(child);

    if (auto it=ar.getChild("Domain"))
    {
      auto type = DomainType::fromString(it->readString("Type"));
      auto domain = Domain::createDomain(type);
      domain->read(*it);
      setDomain(domain);
    }

    for (auto child : readChilds<Group>(ar, "Group"))
      addGroup(child);

    for (auto it : ar.getChilds("xi:include"))
    {
      auto filename = it->readString("href");

      auto stree=StringTree::fromString(Utils::loadTextDocument(filename));
      if (!stree.valid())
        ThrowException("internal error");

      auto group = new Group();
      group->read(stree);
      addGroup(group);
    }

  };


};
  
} //namespace

#endif
