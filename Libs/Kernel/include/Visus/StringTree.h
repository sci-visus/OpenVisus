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


namespace Visus {

//predeclaration
class ObjectStream;

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
    writeString(k1, v1);
  }

  //constructor
  StringTree(String name, String k1, String v1, String k2, String v2) : StringTree(name, k1, v1) {
    writeString(k2, v2);
  }

  //constructor
  StringTree(String name, String k1, String v1, String k2, String v2, String k3, String v3) : StringTree(name, k1, v1, k2, v2) {
    writeString(k3, v3);
  }

  //constructor
  StringTree(String name, String k1, String v1, String k2, String v2, String k3, String v3, String k4, String v4) : StringTree(name, k1, v1, k2, v2, k3, v3) {
    writeString(k4, v4);
  }

  //copy constructor
  StringTree(const StringTree& other){
    operator=(other);
  }

  //destructor
  virtual ~StringTree(){
  }

  //fromXmlString
  bool fromXmlString(String content, bool bEnablePostProcessing = true);

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
  void addChild(SharedPtr<StringTree> child) {
    childs.push_back(child);
  }

#if !SWIG

  //addChild
  void addChild(const StringTree& child) {
    addChild(std::make_shared<StringTree>(child));
  }

  //withChild
  StringTree& withChild(SharedPtr<StringTree> child) {
    addChild(child);
    return *this;
  }

  //withChild
  StringTree& withChild(const StringTree& child) {
    addChild(child);
    return *this;
  }

#endif

  //addChild
  SharedPtr<StringTree> addChild(String name) {
    auto ret = std::make_shared<StringTree>(name);
    addChild(ret);
    return ret;
  }

  //clear
  void clear()
  {
    attributes.clear(); 
    childs.clear();
  }

  //empty
  bool empty() const {
    return attributes.empty() && childs.empty();
  }

  //hasValue
  bool hasValue(String key) const {
    return !readString(key).empty();
  }

public:

  //read
  String readString(String key, String default_value = "") const;

  //writeString
  void writeString(String key, String value);

  //readInt
  bool readBool(String key, bool default_value = false) const {
    return cbool(readString(key, cstring(default_value)));
  }

  //writeBool
  void writeBool(String key, bool value) {
    writeString(key, cstring(value));
  }

  //readInt
  int readInt(String key, int default_value = 0) const {
    return cint(readString(key, cstring(default_value)));
  }

  //writeInt
  void writeInt(String key, int value) {
    writeString(key, cstring(value));
  }

  //readInt64
  Int64 readInt64(String key, Int64 default_value = 0) const {
    return cint64(readString(key, cstring(default_value)));
  }

  //writeInt64
  void writeInt64(String key, Int64 value) {
    writeString(key, cstring(value));
  }

  //readDouble
  double readDouble(String key, double default_value = 0) const {
    return cdouble(readString(key, cstring(default_value)));
  }

  //writeDouble
  void writeDouble(String key, double value) {
    writeString(key, cstring(value));
  }


  //readText
  String readText() const;

  //writeText
  void writeText(const String& text, bool bCData = false) {
    if (bCData)
      childs.push_back(std::make_shared<StringTree>("#cdata-section", "value", text));
    else
      childs.push_back(std::make_shared<StringTree>("#text", "value", text));
  }

  //readText
  String readText(String name) const {
    if (auto child = findChildWithName(name))
      return child->readText();
    else
      return "";
  }

  //writeText
  void writeText(String name, const String& value, bool bCData = false) {
    auto child = std::make_shared<StringTree>(name);
    child->writeText(value, bCData);
    childs.push_back(child);
  }


  //write
  void writeValue(String name, String value) {
    addChild(StringTree(name, "value", value));
  }

  StringTree& withValue(String name, String value) {
    writeValue(name, value);
    return *this;
  }

  //read
  String readValue(String name, String default_value = "")
  {
    auto child = findChildWithName(name);
    return child ? child->readString("value", default_value) : default_value;
  }

  //readObject
  template <class Object>
  bool readObject(String name, Object& obj)
  {
    auto child = getChild(name);
    if (!child) return false;
    obj.readFromObjectStream(ObjectStream(*child, 'r'));
  }

  //writeObject
  template <class Object>
  void writeObject(String name, Object& obj,String TypeName="")
  {
    auto child = addChild(name);
    if (!TypeName.empty())
      child->writeString("TypeName", TypeName);
    obj.writeToObjectStream(ObjectStream(*child, 'w'));
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

  //getChild
  SharedPtr<StringTree> getChild(String name) const {
    for (auto child : childs)
      if (child->name == name)
        return child;
    return SharedPtr<StringTree>();
  }


  //getChild
  std::vector< SharedPtr<StringTree> > getChilds(String name) const {
    std::vector< SharedPtr<StringTree> > ret;
    for (auto child : childs)
      if (child->name == name)
        ret.push_back(child);
    return ret;
  }

  //getFirstChild
  SharedPtr<StringTree> getFirstChild() const {
    return getChild(0);
  }

  //findChildWithName
  StringTree* findChildWithName(String name, StringTree* prev = NULL) const;

  //findAllChildsWithName
  std::vector<StringTree*> findAllChildsWithName(String name, bool bRecursive=true) const;


  //findChildsWithName
  std::vector<StringTree*> findChildsWithName(String name) const {
    return findAllChildsWithName(name, false);
  }

  //getMaxDepth
  int getMaxDepth();

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

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& out);

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& in);

private:

  //toJSONString
  static String toJSONString(const StringTree& stree, int nrec);
 
}; //end class



  /////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ObjectStream
{
public:

  VISUS_CLASS(ObjectStream)

  StringMap run_time_options;

  //constructor
  ObjectStream(StringTree& root, int mode) : mode(0) {
    VisusAssert(mode == 'w' || mode == 'r');
    VisusReleaseAssert(!root.name.empty());
    this->mode = mode;
    this->stack = std::stack<StackItem>();
    this->stack.push(StackItem(&root));
  }

  //destructor
  virtual ~ObjectStream() {
  }

  //getCurrentContext
  StringTree* getCurrentContext() const {
    return stack.top().context;
  }

public:

  //hasAttribute
  bool hasAttribute(String name) {
    return getCurrentContext()->hasAttribute(name);
  }

  //readString
  String readString(String name, String default_value = "") const  {
    return getCurrentContext()->readString(name, default_value);
  }

  //writeString
  void writeString(String name, String value) {
    getCurrentContext()->writeString(name,value);
  }

  //readInt
  bool readBool(String key, bool default_value = false) const {
    return cbool(readString(key, cstring(default_value)));
  }

  //writeBool
  void writeBool(String key, bool value) {
    writeString(key, cstring(value));
  }

  //readInt
  int readInt(String key, int default_value = 0) const {
    return cint(readString(key, cstring(default_value)));
  }

  //writeInt
  void writeInt(String key, int value) {
    writeString(key, cstring(value));
  }

  //readInt64
  Int64 readInt64(String key, Int64 default_value = 0) const {
    return cint64(readString(key, cstring(default_value)));
  }

  //writeInt64
  void writeInt64(String key, Int64 value) {
    writeString(key, cstring(value));
  }

  //readDouble
  double readDouble(String key, double default_value = 0) const {
    return cdouble(readString(key, cstring(default_value)));
  }

  //writeDouble
  void writeDouble(String key, double value) {
    writeString(key, cstring(value));
  }


  //writeObject
  template <class Object>
  void writeObject(Object& obj) {
    obj.writeToObjectStream(*this);
  }

  //readObject
  template <class Object>
  void readObject(Object& obj) {
    obj.readFromObjectStream(*this);
  }

  //writeObject
  template <typename Value>
  void writeObject(String name, Value& value,String TypeName="") {
    getCurrentContext()->writeObject(name, value, TypeName);
  }

  //readObject
  template <typename Value>
  bool readObject(String name, Value& value) {
    return getCurrentContext()->readObject(name, value);
  }

  //write
  void writeValue(String name, String value) {
    getCurrentContext()->writeValue(name, value);
  }

  //read
  String readValue(String name, String default_value = "") {
    return getCurrentContext()->readValue(name, default_value);
  }

  //writeText
  void writeText(const String& value, bool bCData = false) {
    getCurrentContext()->writeText(value, bCData);
  }

  //writeText
  void writeText(String name, const String& value, bool bCData = false) {
    getCurrentContext()->writeText(name, value, bCData);
  }

  //readText
  String readText() {
    return getCurrentContext()->readText();
  }

  //readText
  String readText(String name) {
    return getCurrentContext()->readText(name);
  }

  //addChild
  SharedPtr<StringTree> addChild(String name) {
    return getCurrentContext()->addChild(name);
  }

  //getChild
  SharedPtr<StringTree> getChild(String name) {
    return getCurrentContext()->getChild(name);
  }

private:

  int mode;

  class StackItem
  {
  public:
    StringTree* context;
    std::map<String, StringTree*> next_child;
    StackItem(StringTree* context_ = nullptr) : context(context_) {}
  };
  std::stack<StackItem> stack;

};



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


