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

#ifndef XIDX_PARSABLE_INTERFACE_H_
#define XIDX_PARSABLE_INTERFACE_H_

namespace Visus{

class VISUS_XIDX_API Domain;
class VISUS_XIDX_API DataSource;

VISUS_XIDX_API String XIdxFormatString(const String fmt_str, ...);

/////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API XIdxElement 
{
public:

  //VISUS_XIDX_CLASS(XIdxElement)
  VISUS_CLASS(XIdxElement)

  virtual String getTypeName() {
    return "XIdxElement";
  }

  String name;

  //constructor
  XIdxElement(String name_ = "") : name(name_) {
  }

  //destructor
  virtual ~XIdxElement() {
  }

  //getParent
  virtual XIdxElement* getParent() const {
    return parent;
  }

  //hasChild
  bool hasChild(XIdxElement* child) const {
    return std::find(childs.begin(), childs.end(), child) != childs.end();
  }

  //addEdge
  static void addEdge(XIdxElement* parent, XIdxElement* child) {
    VisusAssert(parent && child && !parent->hasChild(child) && child->parent == nullptr);
    parent->childs.push_back(child);
    child->parent = parent;
  }

  //removeEdge
  static void removeEdge(XIdxElement* parent, XIdxElement* child) {
    VisusAssert(parent && child && parent->hasChild(child) && child->parent == parent);
    Utils::remove(parent->childs, child);
    child->parent = nullptr;
  }

  //getXPathPrefix
  virtual String getXPathPrefix() {
    return concatenate(getParent() ? getParent()->getXPathPrefix() : "/", "/",getTypeName());
  }

  //findChildWithName
  XIdxElement* findChildWithName(String name)
  {
    for (auto child : this->childs) {
      if (child->getTypeName() == name)
        return child;
    }
    return nullptr;
  }

public:

  //write
  virtual void write(Archive& ar) const {
    if (name.size())
      ar.write("Name", name);
  }

  //read
  virtual void read(Archive& ar) {
    name = ar.readString("Name", name);
  }


  //writeChilds
  template <typename T>
  static void writeChild(Archive& ar, String name, const T* child) {
    if (!child) return;
    child->write(*ar.addChild(name));
  }

  //readChilds
  template <typename T>
  static VISUS_NEWOBJECT(T*) readChild(Archive& ar, String name) {
    auto child = ar.getChild(name);
    if (!child) return nullptr;
    auto ret = new T();
    ret->read(*child);
    return ret;
  }

  //readChilds
  template <typename T>
  static std::vector<T*> readChilds(Archive& ar, String name) {
    std::vector<T*> ret;
    for (auto it : ar.getChilds(name))
    {
      auto item = new T();
      item->read(*it);
      ret.push_back(item);
    }
    return ret;
  }



private:

  XIdxElement*              parent = nullptr;
  std::vector<XIdxElement*> childs;


};

} //namespace

#endif //XIDX_PARSABLE_INTERFACE_H_


