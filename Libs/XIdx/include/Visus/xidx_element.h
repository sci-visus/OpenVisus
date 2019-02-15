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

/////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API XIdxElement : public Object
{
public:

  //VISUS_XIDX_CLASS(XIdxElement)
  VISUS_CLASS(XIdxElement)
  virtual String getClassName() {
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
  bool hasChild(SharedPtr<XIdxElement> child) const {
    return std::find(childs.begin(), childs.end(), child) == childs.end();
  }

  //addEdge
  static void addEdge(XIdxElement* parent, SharedPtr<XIdxElement> child) {

    VisusAssert(parent && child && !parent->hasChild(child) && child->parent == nullptr);
    parent->childs.push_back(child);
    child->parent = parent;
  }

  //getXPathPrefix
  virtual String getXPathPrefix() {
    return StringUtils::format()<<(getParent() ? getParent()->getXPathPrefix() : "/") << "/" << getClassName();
  }

  //findChildWithName
  SharedPtr<XIdxElement> findChildWithName(String name)
  {
    for (auto child : this->childs) {
      if (child->getClassName() == name)
        return child;
    }
    return nullptr;
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) {
    ostream.writeInline("Name", name);
  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) {
    name = istream.readInline("Name", name);
  }

  //writeChilds
  template <typename T>
  void writeChild(ObjectStream& ostream,String name, SharedPtr<T> child) {
    if (!child) return;
    ostream.pushContext(name);
    child->writeToObjectStream(ostream);
    ostream.popContext(name);
  }

  //readChilds
  template <typename T>
  SharedPtr<T> readChild(ObjectStream& istream, String name) {
    
    if (!istream.pushContext(name))
      return SharedPtr<T>();

    auto ret = std::make_shared<T>();
    ret->readFromObjectStream(istream);
    istream.popContext(name);
    return ret;
  }

private:

  XIdxElement*                          parent = nullptr;
  std::vector< SharedPtr<XIdxElement> > childs;

};

} //namespace

#endif //XIDX_PARSABLE_INTERFACE_H_


