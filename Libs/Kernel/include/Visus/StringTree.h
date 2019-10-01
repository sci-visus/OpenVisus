/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#ifndef VISUS_STRINGTREE_H__
#define VISUS_STRINGTREE_H__

#include <Visus/Kernel.h>
#include <Visus/StringMap.h>
#include <Visus/BigInt.h>
#include <Visus/StringMap.h>
#include <Visus/Singleton.h>

#include <stack>
#include <vector>
#include <iostream>
#include <vector>
#include <functional>

namespace Visus {

///////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API StringTree 
{
public:

  VISUS_CLASS(StringTree)

  //name
  String name;

  //attributes
  std::vector< std::pair<String, String> > attributes;

  //childs
  std::vector< SharedPtr<StringTree> > childs;

  // constructor
  StringTree(String name_ = "") : name(name_){
  }

  //constructor
  StringTree(String name, String k1, String v1) : StringTree(name) {
    write(k1, v1);
  }

  //constructor
  StringTree(String name, String k1, String v1, String k2, String v2) : StringTree(name, k1, v1) {
    write(k2, v2);
  }

  //constructor
  StringTree(String name, String k1, String v1, String k2, String v2, String k3, String v3) : StringTree(name, k1, v1, k2, v2) {
    write(k3, v3);
  }

  //constructor
  StringTree(String name, String k1, String v1, String k2, String v2, String k3, String v3, String k4, String v4) : StringTree(name, k1, v1, k2, v2, k3, v3) {
    write(k4, v4);
  }

  //copy constructor
  StringTree(const StringTree& other){
    operator=(other);
  }

  //destructor
  virtual ~StringTree(){
  }

  //valid
  bool valid() const {
    return !name.empty();
  }

  //fromString
  static StringTree fromString(String content, bool bEnablePostProcessing = true);

  //operator=
  StringTree& operator=(const StringTree& other) {
    this->name = other.name;
    this->attributes = other.attributes;
    this->childs.clear();
    for (int I = 0; I < other.childs.size(); I++)
      this->childs.push_back(std::make_shared<StringTree>(*other.childs[I]));
    return *this;
  }

  //hasAttribute
  bool hasAttribute(String name) const
  {
    for (int I = 0; I < attributes.size(); I++) {
      if (attributes[I].first == name)
        return true;
    }
    return false;
  }

  //getAttribute
  String getAttribute(String name, String default_value="") const
  {
    for (int I = 0; I < this->attributes.size(); I++) {
      if (attributes[I].first == name)
        return attributes[I].second;
    }
    return default_value;
  }

  //setAttribute
  void setAttribute(String name, String value)
  {
    for (int I = 0; I < this->attributes.size(); I++) {
      if (attributes[I].first == name) {
        attributes[I].second = value;
        return;
      }
    }
    attributes.push_back(std::make_pair(name,value));
  }

  //getChilds
  const std::vector< SharedPtr<StringTree> >& getChilds() const {
    return childs;
  }

  //addChild
  StringTree& addChild(SharedPtr<StringTree> child) {
    childs.push_back(child);
    return *this;
  }

#if !SWIG
  //addChild
  StringTree& addChild(const StringTree& child) {
    return addChild(std::make_shared<StringTree>(child));
  }
#endif

  //addChild
  SharedPtr<StringTree> addChild(String name) {
    auto cursor = NormalizeW(this, name);
    auto child = std::make_shared<StringTree>(name);
    cursor->addChild(child);
    return child;
  }


  //clear
  void clear()
  {
    attributes.clear(); 
    childs.clear();
  }

  //hasValue
  bool hasValue(String key) const {
    return !readString(key).empty();
  }

public:

  //write
  StringTree& write(String key, String value);

  //write
  StringTree& write(String key, const char* value) {
    return write(key, String(value));
  }

  //writeString
  StringTree& writeString(String key, String value) {
    return write(key, value);
  }

  //write
  StringTree& write(String key, bool value) {
    return write(key, cstring(value));
  }

  //write
  StringTree& write(String key, int value) {
    return write(key, cstring(value));
  }

  //write
  StringTree& write(String key, Int64 value) {
    return write(key, cstring(value));
  }

  //write
  StringTree& write(String key, double value) {
    return write(key, cstring(value));
  }

  //write
  template <class Value>
  StringTree& write(String key, const Value& value) {
    return write(key, value.toString());
  }

  //read
  String read(String key, String default_value = "") const;

  //readString
  String readString(String key, String default_value = "") const {
    return read(key, default_value);
  }

  //readInt
  bool readBool(String key, bool default_value = false) const {
    return cbool(read(key, cstring(default_value)));
  }

  //readInt
  int readInt(String key, int default_value = 0) const {
    return cint(read(key, cstring(default_value)));
  }

  //readInt64
  Int64 readInt64(String key, Int64 default_value = 0) const {
    return cint64(read(key, cstring(default_value)));
  }

  //readDouble
  double readDouble(String key, double default_value = 0) const {
    return cdouble(read(key, cstring(default_value)));
  }

  //readText
  String readText() const;

  //writeText
  StringTree& writeText(const String& text, bool bCData = false) {
    if (bCData)
      childs.push_back(std::make_shared<StringTree>("#cdata-section", "value", text));
    else
      childs.push_back(std::make_shared<StringTree>("#text", "value", text));
    return *this;
  }

  //readText
  String readText(String name) const {
    if (auto child = getChild(name))
      return child->readText();
    else
      return "";
  }

  //writeText
  StringTree& writeText(String name, const String& value, bool bCData = false) {

    auto cursor = NormalizeW(this,name);
    cursor->addChild(name)->writeText(value,bCData);
    return *this;
  }

  //write
  StringTree& writeValue(String name, String value) {
    auto cursor = NormalizeW(this, name);
    cursor->addChild(name)->writeString("value", value);
    return *this;
  }

  //read
  String readValue(String name, String default_value = "")
  {
    auto child = getChild(name);
    return child ? child->readString("value", default_value) : default_value;
  }

  //readObject
  template <class Object>
  bool readObject(String name, Object& obj)
  {
    auto child = getChild(name);
    if (!child) return false;
    obj.readFrom(*child);
    return true;
  }

  //writeObject
  template <class Object>
  StringTree& writeObject(String name, Object& obj,String TypeName="")
  {
    auto cursor=NormalizeW(this,name);
    auto child = cursor->addChild(name);
    if (!TypeName.empty())
      child->write("TypeName", TypeName);
    obj.writeTo(*child);
    return *this;
  }

public:

  //getNumberOfChilds
  int getNumberOfChilds() const {
    return (int)childs.size();
  }

  //getChild
  SharedPtr<StringTree> getChild(int I) const {
    return childs[I];
  }

  //getFirstChild
  SharedPtr<StringTree> getFirstChild() const {
    return getChild(0);
  }

  //getChild
  SharedPtr<StringTree> getChild(String name) const;

  //getChild
  std::vector< SharedPtr<StringTree> > getChilds(String name) const;

  //getAllChilds
  std::vector<StringTree*> getAllChilds(String name) const;

  //internal use only
  static StringTree postProcess(const StringTree& src);

public:

  //isHashNode
  bool isHashNode() const{
    return !name.empty() && name[0] == '#';
  }

  //isCommentNode
  bool isCommentNode() const{
    return name == "#comment";
  }

  //addCommentNode
  void addCommentNode(String text){
    childs.push_back(std::make_shared<StringTree>("#comment", "value", text));
  }

public:

  //toXmlString
  String toXmlString() const;

  //toJSONString
  String toJSONString() const {
    return toJSONString(*this, 0);
  }

  //toString
  String toString() const {
    return toXmlString();
  }

private:

  //toJSONString
  static String toJSONString(const StringTree& stree, int nrec);
 
  //NormalizeR
  static const StringTree* NormalizeR(const StringTree* cursor, String& key);

  //NormalizeW
  static StringTree* NormalizeW(StringTree* cursor, String& key);

}; //end class

template <class Object>
inline StringTree EncodeObject(const Object* obj, String TypeName)
{
  if (!obj) return StringTree();
  StringTree ret(TypeName);
  obj->writeTo(ret);
  return ret;
}


template <class Object>
inline StringTree EncodeObject(const Object* obj) {
  return obj ? EncodeObject(obj,obj->getTypeName()) : StringTree();
}

template <class Object>
inline Object* DecodeObject(StringTree in,std::function<Object*(String)> creator)
{
  auto TypeName = in.name;
  UniquePtr<Object> ret(creator(TypeName));
  if (!ret) return nullptr;
  ret->readFrom(in);
  return ret.release();
}

//////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ConfigFile : public StringTree
{
public:

  //constructor
  ConfigFile(String name = "ConfigFile") : StringTree(name) {
  }

  //destructor
  ~ConfigFile() {
  }

  //getFilename
  String getFilename() const {
    return filename;
  }

  //load
  bool load(String filename, bool bEnablePostProcessing = true);

  //reload
  bool reload(bool bEnablePostProcessing = true) {
    return load(filename, bEnablePostProcessing);
  }

  //save
  bool save();


private:

  String filename;

};

//////////////////////////////////////////////////////////////////////
#if !SWIG
namespace Private {
class VISUS_KERNEL_API VisusConfig : public ConfigFile
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(VisusConfig)

  //constructor
  VisusConfig() : ConfigFile("visus_config") {
  }
};
} //namespace Private
#endif


} //namespace Visus
 
#endif //VISUS_STRINGTREE_H__


