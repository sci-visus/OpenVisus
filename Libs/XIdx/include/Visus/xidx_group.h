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

  VISUS_CLASS(Group)

  // node info
  int                                 domain_index = 0;
  String                              file_pattern;
  GroupType                           group_type;
  VariabilityType                     variability_type;

  //down nodes
  SharedPtr<Domain>                   domain;
  std::vector<SharedPtr<Group> >      groups;
  std::vector<SharedPtr<Variable> >   variables;
  std::vector<SharedPtr<DataSource> > data_sources;
  std::vector<SharedPtr<Attribute> >  attributes;

  //constructor
  Group(String name_=""){
    name= name_;
  }
  
  //setDomain
  inline void setDomain(SharedPtr<Domain> value) { 
    addEdge(this, value);
    this->domain = value; 
  }

  //addVariable
  void addVariable(SharedPtr<Variable> value) {
    addEdge(this, value);
    variables.push_back(value);
  }

  //addAttribute
  void addAttribute(SharedPtr<Attribute> value) {
    addEdge(this, value);
    attributes.push_back(value);
  }
  
  //addDataSource
  void addDataSource(SharedPtr<DataSource> value) {
    addEdge(this, value);
    data_sources.push_back(value);
  }

  //addGroup
  void addGroup(SharedPtr<Group> value)
  {
    if(value->variability_type == VariabilityType::VARIABLE_VARIABILITY_TYPE)
      value->domain_index = (int)groups.size();
    
    addEdge(this,value);
    groups.push_back(value);
  }
  
  //getXPathPrefix
  virtual String getXPathPrefix() override {
    return StringUtils::format() << (getParent()? getParent()->getXPathPrefix() : "//Xidx") << "/Group" << "[@Name=\"" + name + "\"]";
  }

public:

  //load
  static SharedPtr<Group> load(String filename)
  {
    StringTree stree;
    if (!stree.loadFromXml(Utils::loadTextDocument(filename)))
      return SharedPtr<Group>();

    ObjectStream istream(stree, 'r');
    auto ret = std::make_shared<Group>("root");
    ret->readFromObjectStream(istream);
    return ret;
  }

  //save
  static bool save(String filename, SharedPtr<Group> root)
  {
    StringTree stree(root->getVisusClassName());
    ObjectStream ostream(stree, 'w');
    root->writeToObjectStream(ostream);
    Utils::saveTextDocument(filename, stree.toString());
    return true;
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    XIdxElement::writeToObjectStream(ostream);

    ostream.writeInline("Name", name);
    ostream.writeInline("Type", group_type.toString());
    ostream.writeInline("VariabilityType", variability_type.toString());
    ostream.writeInline("FilePattern", file_pattern.c_str());
    ostream.writeInline("DomainIndex", Visus::cstring(domain_index));

    for (auto child : data_sources)
      writeChild<DataSource>(ostream, "DataSource", child);

    writeChild<Domain>(ostream, "Domain",domain);

    for (auto child : attributes)
      writeChild<Attribute>(ostream, "Attribute", child);

    for (auto child : variables)
      writeChild<Variable>(ostream, "Variable",child);

    for (auto child : groups) 
    {
      if (file_pattern.empty())
      {
        writeChild<Group>(ostream, "Group", child);
      }
      else
      {
        String filename = FormatString(file_pattern + "/meta.xidx", child->domain_index);

        ostream.pushContext("xi:include");
        ostream.writeInline("href", filename.c_str());
        ostream.writeInline("xpointer", "xpointer(//Xidx/Group/Group)");
        ostream.popContext("xi:include");

        {
          StringTree stree(child->getVisusClassName());
          ObjectStream ostream(stree, 'w');
          child->writeToObjectStream(ostream);
          auto content = stree.toString();
          Utils::saveTextDocument(filename,content);
        }
      }
    }
  };

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override
  {
    XIdxElement::readFromObjectStream(istream);

    this->group_type = GroupType::fromString(istream.readInline("Type"));
    this->variability_type = VariabilityType::fromString(istream.readInline("VariabilityType"));
    this->file_pattern = istream.readInline("FilePattern");
    this->domain_index = cint(istream.readInline("DomainIndex"));

    while (auto child = readChild<DataSource>(istream,"DataSource"))
      addDataSource(child);

    while (auto child = readChild<Attribute>(istream,"Attribute"))
      addAttribute(child);

    while (auto child = readChild<Variable>(istream,"Variable"))
      addVariable(child);

    if (istream.pushContext("Domain"))
    {
      auto type = DomainType::fromString(istream.readInline("Type"));
      auto child=Domain::createDomain(type);
      child->readFromObjectStream(istream);
      istream.popContext("Domain");
      setDomain(child);
    }

    while (auto child = readChild<Group>(istream,"Group"))
      addGroup(child);

    while (istream.pushContext("xi:include"))
    {
      auto filename = istream.readInline("href");

      StringTree stree;
      if (!stree.loadFromXml(Utils::loadTextDocument(filename)))
        ThrowException("internal error");

      auto child = std::make_shared<Group>();
      {
        ObjectStream istream(stree, 'r');
        child->readFromObjectStream(istream);
      }
      istream.popContext("xi:include");

      addGroup(child);
    }

  };

private:

  //FormatString
  static String FormatString(const String fmt_str, ...);
  
};
  
} //namespace

#endif
