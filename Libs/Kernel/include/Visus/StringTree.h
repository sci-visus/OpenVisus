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
#include <Visus/ObjectStream.h>


namespace Visus {

///////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API StringTree 
{
public:

  VISUS_CLASS(StringTree)

  //name
  String name;

  //attributes
  StringMap attributes;

  // constructor
  StringTree(String name_ = "") : name(name_){
  }

  //constructor
  StringTree(String name_, String attr_k1, String attr_v1) : name(name_){
    writeString(attr_k1, attr_v1);
  }

  //constructor
  StringTree(String name_, String attr_k1, String attr_v1, String attr_k2, String attr_v2) : name(name_)
  {
    writeString(attr_k1, attr_v1); writeString(attr_k2, attr_v2);
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
  StringTree& operator=(const StringTree& other);

  //clear
  void clear()
  {
    attributes.clear(); childs.clear();
  }

  //empty
  bool empty() const {
    return attributes.empty() && childs.empty();
  }

  //hasValue
  bool hasValue(String key) const {
    return !readString(key).empty();
  }

  //read
  String readString(String key, String default_value = "") const;

  //writeString
  void writeString(String key, String value);

  //readInt
  int readInt(String key, int default_value = 0) const {
    return cint(readString(key, cstring(default_value)));
  }

  //writeInt
  void writeInt(String key, int value) {
    writeString(key, cstring(value));
  }

  //readInt
  bool readBool(String key, bool default_value = false) const {
    return cbool(readString(key, cstring(default_value)));
  }

  //writeBool
  void writeBool(String key, bool value) {
    writeString(key, cstring(value));
  }

  //readBigInt
  BigInt readBigInt(String key, BigInt default_value = 0) const {
    return cbigint(readString(key, cstring(default_value)));
  }

  //writeBigInt
  void writeBigInt(String key, BigInt value) {
    writeString(key, cstring(value));
  }

  //getNumberOfChilds
  int getNumberOfChilds() const {
    return (int)childs.size();
  }

  //getChilds
  const std::vector<StringTree*> getChilds() const {
    std::vector<StringTree*> ret;
    for (auto child : this->childs)
      ret.push_back(child.get());
    return ret;
  }

  //getChild
  const StringTree& getChild(int index) const {
    return *childs[index];
  }

  //getChild
  StringTree& getChild(int index) {
    return *childs[index];
  }

  //getFirstChild
  const StringTree& getFirstChild() const {
    return *childs.front();
  }

  //getFirstChild
  StringTree& getFirstChild() {
    return *childs.front();
  }

  //getLastChild
  const StringTree& getLastChild() const {
    return *childs.back();
  }

  //getLastChild
  StringTree& getLastChild(){
    return *childs.back();
  }

  //addChild
  StringTree* addChild(const StringTree& child) {
    childs.push_back(std::make_shared<StringTree>(child)); 
    return childs.back().get();
  }

  //findChildWithName
  StringTree* findChildWithName(String name, StringTree* prev = NULL) const;

  //findAllChildsWithName
  std::vector<StringTree*> findAllChildsWithName(String name, bool bRecursive) const;

  //getMaxDepth
  int getMaxDepth();

  //inheritAttributeFrom
  void inheritAttributeFrom(const StringTree& other)  
  {
    for (auto it : other.attributes)
    {
      String key   = it.first;
      String value = it.second;
      if (this->attributes.find(key) == this->attributes.end())
        this->attributes.setValue(key,value);
    }
  }

  //internal use only
  static StringTree postProcess(const StringTree& src);

public:

  //isHashNode
  bool isHashNode() const{
    return !name.empty() && name[0] == '#';
  }

  //isTextNode
  bool isTextNode() const{
    return name == "#text";
  }

  //addTextNode
  void addTextNode(String text){
    addChild(StringTree("#text", "value", text));
  }

  //isCDataSectionNode
  bool isCDataSectionNode() const
  {
    return name == "#cdata-section";
  }

  //addCDataSectionNode
  void addCDataSectionNode(String text){
    addChild(StringTree("#cdata-section", "value", text));
  }

  //isCommentNode
  bool isCommentNode() const{
    return name == "#comment";
  }

  //addCommentNode
  void addCommentNode(String text){
    addChild(StringTree("#comment", "value", text));
  }

  //collapseTextAndCData (backward compatible, collect all text in TextNode and CDataSectionNode)
  String collapseTextAndCData() const;

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
  void writeToObjectStream(ObjectStream& ostream);

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream);

private:

  //childs
  std::vector< SharedPtr<StringTree> > childs;

  //toJSONString
  static String toJSONString(const StringTree& stree, int nrec);
 
}; //end class


} //namespace Visus
 
#endif //VISUS_STRINGTREE_H__


