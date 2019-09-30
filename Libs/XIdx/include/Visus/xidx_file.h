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

#ifndef XIDX_FILE_H_
#define XIDX_FILE_H_


namespace Visus{

///////////////////////////////////////////////////////////////////
class VISUS_XIDX_API XIdxFile : public XIdxElement
{
public:

  VISUS_CLASS(XIdxFile)

  std::vector<Group*>      groups;
  String                   file_pattern;

  //constructor
  XIdxFile(String name_="") {
    name= name_;
  }

  //destructor
  virtual ~XIdxFile() {
    for (auto it : groups)
      delete it;
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
    return StringUtils::format() << (getParent()? getParent()->getXPathPrefix() : "//Xidx") << "[@Name=\"" + name + "\"]";
  }

  //getGroup
  Group* getGroup(GroupType type) const
  {
    for (auto g: groups)
      if(g->group_type.value == type.value)
        return g;
    return nullptr;
  }

public:

  //load
  static VISUS_NEWOBJECT(XIdxFile*) load(String filename)
  {
    StringTree in=StringTree::fromString(Utils::loadTextDocument(filename));
    if (!in.valid()) return nullptr;
    auto ret = new XIdxFile("");
    ret->readFrom(in);
    return ret;
  }

  //save
  bool save(String filename)
  {
    StringTree out(this->getTypeName());
    this->writeTo(out);
    Utils::saveTextDocument(filename, out.toString());
    return true;
  }

public:

  //writeTo
  virtual void writeTo(StringTree& out) const override
  {
    XIdxElement::writeTo(out);

    //out.writeString("Name", name);

    for (auto child : groups) 
    {
      if (file_pattern.empty())
      {
        out.writeObject("Group",*child);
      }
      else
      {
        String filename = XIdxFormatString(file_pattern + "/meta.xidx", child->domain_index);

        if(auto xi_include = out.addChild("xi:include"))
        {
          xi_include->writeString("href", filename.c_str());
          xi_include->writeString("xpointer", "xpointer(//Xidx/Group/Group)");
        }

        {
          StringTree out(child->getTypeName());
          child->writeTo(out);
          auto content = out.toString();
          Utils::saveTextDocument(filename,content);
        }
      }
    }
  };

  //readFrom
  virtual void readFrom(StringTree& in) override
  {
    XIdxElement::readFrom(in);

    for (auto child : in.getChilds("Group"))
    {
      auto group = new Group();
      group->readFrom(*child);
      addGroup(group);
    }

    for (auto xi_include : in.getChilds("xi:include"))
    {
      auto filename = xi_include->readString("href");
      StringTree in=StringTree::fromString(Utils::loadTextDocument(filename));
      if (!in.valid())
        ThrowException("internal error");

      auto child = new Group();
      child->readFrom(in);
      addGroup(child);
    }

  };

};
  
} //namespace

#endif
