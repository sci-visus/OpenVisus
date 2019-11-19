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
    return concatenate(getParent()? getParent()->getXPathPrefix() : "//Xidx", "[@Name=\"",name,"\"]");
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
    ret->read(in);
    return ret;
  }

  //save
  void save(String filename)
  {
    StringTree out(this->getTypeName());
    this->write(out);
    Utils::saveTextDocument(filename, out.toString());
  }

public:

  //write
  virtual void write(Archive& ar) const override
  {
    XIdxElement::write(ar);

    //ar.write("Name", name);

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

    for (auto child : readChilds<Group>(ar, "Group"))
      addGroup(child);

    for (auto it : ar.getChilds("xi:include"))
    {
      auto stree=StringTree::fromString(Utils::loadTextDocument(it->readString("href")));
      if (!stree.valid())
        ThrowException("internal error");

      auto child = new Group();
      child->read(stree);
      addGroup(child);
    }

  };

};
  
} //namespace

#endif
