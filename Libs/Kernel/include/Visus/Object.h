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

#ifndef VISUS_OBJECT_H__
#define VISUS_OBJECT_H__

#include <Visus/Kernel.h>
#include <Visus/StringMap.h>
#include <Visus/Singleton.h>

#include <stack>
#include <vector>
#include <iostream>

namespace Visus {

//predeclaration
class Object;
class StringTree;

  /////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ObjectStream 
{
public:

  VISUS_CLASS(ObjectStream)

  StringMap run_time_options;

  //constructor
  ObjectStream() : mode(0) {
  }

  //constructor
  ObjectStream(StringTree& root_,int mode_) : mode(0) {
    open(root_,mode_);
  }

  //destructor
  virtual ~ObjectStream(){
    close();
  }

  //open
  void open(StringTree& root,int mode);

  //close
  void close();

  //getCurrentDepth
  int getCurrentDepth(){
    return (int)stack.size();
  }

  //getCurrentContext
  StringTree* getCurrentContext(){
    return stack.top().context;
  }

  // pushContext
  bool pushContext(String context_name);

  //popContext
  bool popContext(String context_name);

  //writeInline
  void writeInline(String name, String value);

  //readInline
  String readInline(String name, String default_value = "");

  //writeTypeName
  void writeTypeName(const Object& obj);

  //readTypeName
  String readTypeName();

  //write
  void write(String name, String value);

  //read
  String read(String name, String default_value = "");

  //writeObject
  void writeObject(String name, Object* obj);

  //writeText
  void writeText(const String& value, bool bCData = false);

  //readText
  String readText();

  //readObject
  VISUS_NEWOBJECT(Object*) readObject(String name);

  //readObject
  template <class ClassName>
  VISUS_NEWOBJECT(ClassName*) readObject(String name);

private:

  int mode;

  class StackItem
  {
  public:
    StringTree*                  context;
    std::map<String,StringTree*> next_child;
    StackItem(StringTree* context_=nullptr) : context(context_) {}
  };
  std::stack<StackItem> stack;

};


///////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Object 
{
public:

  VISUS_CLASS(Object)

  //constructor
  Object(){
  }

  //destructor
  virtual ~Object() {
  }

  //getOsDependentTypeName
  virtual String getOsDependentTypeName() const {
    return typeid(*this).name();
  }

  //default conversion
  virtual bool              toBool() const { return true; }
  virtual int               toInt() const { return    0; }
  virtual Int64             toInt64() const { return    0; }
  virtual double            toDouble() const { return  0.0; }
  virtual String            toString() const { return   ""; }

  //toXmlString
  String toXmlString() const;

public:

  //writeToObjectStream (to implement if needed!)
  virtual void writeToObjectStream(ObjectStream& ostream) {
    VisusAssert(false);
  }

  //readFromObjectStream (to implement if needed!)
  virtual void readFromObjectStream(ObjectStream& istream) {
    VisusAssert(false);
  }
  
  //writeToSceneObjectStream (to implement if needed!)
  virtual void writeToSceneObjectStream(ObjectStream& ostream) {
    VisusAssert(false);
  }
  
  //readFromObjectStream (to implement if needed!)
  virtual void readFromSceneObjectStream(ObjectStream& istream) {
    VisusAssert(false);
  }

};

  
//readObject
template <class ClassName>
inline VISUS_NEWOBJECT(ClassName*) ObjectStream::readObject(String name) {
  Object* obj=readObject(name);
  ClassName* ret=dynamic_cast<ClassName*>(obj);
  if (!ret && obj) delete obj;
  return ret;
}
  

/////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ObjectEncoder : public Object
{
public:

  VISUS_CLASS(ObjectEncoder)

  //constructor
  ObjectEncoder(){
  }

  //destructor
  virtual ~ObjectEncoder(){
  }

  //encode
  virtual String encode(const Object* obj) = 0;

  //decode
  virtual SharedPtr<Object> decode(const String& src) = 0;

};


///////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ObjectCreator
{
public:

  VISUS_CLASS(ObjectCreator)

  //constructor
  ObjectCreator(){
  }

  //destructor
  virtual ~ObjectCreator(){
  }

  //createInstance
  virtual VISUS_NEWOBJECT(Object*) createInstance() {
    ThrowException("internal error, you forgot to implement createInstance");
    return nullptr;
  }
};


///////////////////////////////////////////////////////////////////////////////
template <class ClassName>
class CppObjectCreator : public ObjectCreator
{
public:

  //constructor
  CppObjectCreator(){
  }

  //destructor
  virtual ~CppObjectCreator(){
  }

  //createInstance
  virtual VISUS_NEWOBJECT(Object*) createInstance() override{
    return new ClassName();
  }

private:

  VISUS_NON_COPYABLE_CLASS(CppObjectCreator)
};

  ///////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ObjectFactory
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(ObjectFactory)

  //desgtructor
  ~ObjectFactory() {
  }

  //registerObjectClass
  void registerObjectClass(String portable_typename,String os_typename,SharedPtr<ObjectCreator> creator)
  {
    VisusAssert(creators.find(portable_typename)==creators.end());
    creators[portable_typename]=creator;
    portable_typenames.setValue(os_typename,portable_typename);
  }

  //registerObjectClass
  void registerObjectClass(String portable_typename, String os_typename, ObjectCreator* VISUS_DISOWN(creator)) {
    return registerObjectClass(portable_typename,os_typename, SharedPtr<ObjectCreator>(creator));
  }

  //flushObjectCreators (needed for swig)
  void flushObjectCreators()
  {
    creators.clear();
    portable_typenames.clear();
  }

  //VISUS_REGISTER_OBJECT_CLASS
  #define VISUS_REGISTER_OBJECT_CLASS(ClassName) \
    Visus::ObjectFactory::getSingleton()->registerObjectClass(#ClassName,typeid(ClassName).name(),std::make_shared<CppObjectCreator<ClassName> >()) \
    /*--*/

  //createInstance
  VISUS_NEWOBJECT(Object*) createInstance(String portable_typename,bool bCanFail=false)
  {
    auto it=creators.find(portable_typename);
    if (it==creators.end()) {VisusAssert(bCanFail);return nullptr;}
    return it->second->createInstance();
  }

  //createInstance
  template <class ClassName>
  VISUS_NEWOBJECT(ClassName*) createInstance(String portable_typename,bool bCanFail=false)
  {
    Object* obj=createInstance(portable_typename,bCanFail);
    if (ClassName* ret=dynamic_cast<ClassName*>(obj)) return ret;
    if (obj) delete obj;
    return nullptr;
  }

  //getPortableTypeName
  String getPortableTypeName(const Object& instance)
  {
    String os_dependent_typename=instance.getOsDependentTypeName();
    VisusAssert(portable_typenames.hasValue(os_dependent_typename));
    return portable_typenames.getValue(os_dependent_typename);
  }

private:

  //constructor
  ObjectFactory(){
  }

  std::map<String, SharedPtr<ObjectCreator> > creators;
  StringMap                                   portable_typenames;

};

inline bool       cbool    (const SharedPtr<Object>& obj) {return obj.get() ?obj->toBool  ():false;} 
inline int        cint     (const SharedPtr<Object>& obj) {return obj.get() ?obj->toInt   ():0;}
inline Int64      cint64   (const SharedPtr<Object>& obj) {return obj.get() ?obj->toInt64():0;}
inline double     cdouble  (const SharedPtr<Object>& obj) {return obj.get() ?obj->toDouble():0.0;}
inline String     cstring  (const SharedPtr<Object>& obj) {return obj.get() ?obj->toString():"";}


/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API BoolObject : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(BoolObject)

  //constructor
  BoolObject(bool value_=false) : value(value_){
  }

  //destructor
  virtual ~BoolObject(){
  }

  virtual bool       toBool() const override { return value; }
  virtual int        toInt() const override { return value ? 1 : 0; }
  virtual Int64      toInt64() const override { return value ? 1 : 0; }
  virtual double     toDouble() const override { return value ? 1.0 : 0.0; }
  virtual String     toString() const override { return value ? "True" : "False"; }

  //getValue
  bool getValue() const {
    return value;
  }

  //setValue
  void setValue(bool value) {
    this->value=value;
  }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  bool value;

};

/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API IntObject : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(IntObject)

  //constructor
  IntObject(int value_=0) : value(value_){
  }

  //destructor
  virtual ~IntObject(){
  }

  virtual bool       toBool() const override { return value ? true : false; }
  virtual int        toInt() const override { return value; }
  virtual Int64      toInt64() const override { return value; }
  virtual double     toDouble() const override { return value; }
  virtual String     toString() const override { return cstring(value); }

  //getValue
  int getValue() const{
    return value;
  }

  //setValue
  void setValue(int value) {
    this->value=value;
  }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  int value;

};

/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Int64Object : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(Int64Object)

  //constructor
  Int64Object(Int64 value_=0) : value(value_){
  }

  //destructor
  virtual ~Int64Object() {
  }

  //getValue
  Int64 getValue() {
    return value;
  }

  //setValue
  void setValue(int value) {
    this->value=value;
  }

  virtual bool       toBool() const override { return value ? true : false; }
  virtual int        toInt() const override { return (int)value; }
  virtual Int64      toInt64() const override { return value; }
  virtual double     toDouble() const override { return (double)value; }
  virtual String     toString() const override { std::ostringstream o; o << value; return o.str(); }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  Int64 value;
};

/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API DoubleObject : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(DoubleObject)

  //constructor
  DoubleObject(double value_=0) : value(value_) {
  }

  //destructor
  virtual ~DoubleObject(){
  }

  virtual bool       toBool() const override { return value ? true : false; }
  virtual int        toInt() const override { return (int)value; }
  virtual Int64      toInt64() const override { return (Int64)value; }
  virtual double     toDouble() const override { return value; }
  virtual String     toString() const override { return cstring(value); }

  //getValue
  double getValue() const{
    return value;
  }

  //setValue
  virtual void setValue(double value) {
    this->value=value;
  }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  double value;

};


/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API StringObject : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(StringObject)

  //constructor
  StringObject(String value_="") : value(value_){
  }

  //destructor
  virtual ~StringObject(){
  }

  virtual bool          toBool() const override { return cbool(value); }
  virtual int           toInt() const override { return cint(value); }
  virtual Int64         toInt64() const override { return cint64(value); }
  virtual double        toDouble() const override { return cdouble(value); }
  virtual String        toString() const override { return value; }

  //getValue
  String getValue() const{
    return value;
  }

  //setValue
  void setValue(String value) {
    this->value=value;
  }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  String value;

};


/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ListObject : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(ListObject)

  typedef std::vector< SharedPtr<Object> >::iterator       iterator;
  typedef std::vector< SharedPtr<Object> >::const_iterator const_iterator;

  //constructor
  ListObject(){
  }

  //constructor
  ListObject(std::vector< SharedPtr<Object> > vector_) : vector(vector_) {
  }

  //destructor
  virtual ~ListObject(){
  }

  //size
  int size() const{
    return (int)vector.size();
  }

  //clear
  void clear(){
    vector.clear();
  }

  //getAt
  SharedPtr<Object> getAt(int index){
    return vector[index];
  }

  //setAt
  void setAt(int index, SharedPtr<Object> value){
    vector[index] = value;
  }

  //push_back
  void push_back(SharedPtr<Object> value){
    vector.push_back(value);
  }

  iterator begin() { return vector.begin(); }
  iterator end() { return vector.end(); }

  const_iterator begin() const { return vector.begin(); }
  const_iterator end() const { return vector.end(); }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  std::vector< SharedPtr<Object> > vector;

};

///////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API DictObject : public Object
{
public:

  VISUS_NON_COPYABLE_CLASS(DictObject)

  typedef std::map<String, SharedPtr<Object> >::iterator       iterator;
  typedef std::map<String, SharedPtr<Object> >::const_iterator const_iterator;

  //constructor 
  DictObject(){
  }

  //constructor
  DictObject(std::map<String,SharedPtr<Object> > map_) : map(map_) {
  }

  //destructor
  virtual ~DictObject(){
  }

  //hasattr
  virtual bool hasattr(String name) const{
    return map.find(name) != map.end();
  }

  //getattr
  virtual SharedPtr<Object> getattr(String name, SharedPtr<Object> default_value = SharedPtr<Object>()) const
  {
    const_iterator it = map.find(name);
    if (it == map.end()) return default_value;
    return it->second;
  }

  //remattr
  virtual void remattr(String name)
  {
    iterator it = map.find(name);
    if (it == map.end()) return;
    map.erase(it);
  }

  //setattr
  virtual void setattr(String name, SharedPtr<Object> value)
  {
    map[name] = value;
  }

  iterator       begin() { return map.begin(); }
  iterator       end  () { return map.end(); }

  const_iterator begin() const { return map.begin(); }
  const_iterator end  () const { return map.end(); }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  std::map<String, SharedPtr<Object> > map;

};

} //namespace Visus


#endif //VISUS_OBJECT_H__


