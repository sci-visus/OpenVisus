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

#ifndef VISUS_DATAFLOW_MESSAGE_H__
#define VISUS_DATAFLOW_MESSAGE_H__

#include <Visus/DataflowModule.h>
#include <Visus/Semaphore.h>
#include <Visus/CriticalSection.h>
#include <Visus/ObjectStream.h>
#include <Visus/Array.h>

#include <set>
#include <type_traits>

namespace Visus {

//predeclaration
class Node;


///////////////////////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API ReturnReceipt
{
public:

  VISUS_NON_COPYABLE_CLASS(ReturnReceipt)

  typedef void Signer;

  //constructor
  ReturnReceipt();

  //destructor
  ~ReturnReceipt();

  //isReady
  bool isReady();

  //waitReady
  void waitReady(SharedPtr<Semaphore> ready_semaphore);

  //needSignature
  void needSignature(Signer* signer);

  //addSignature
  void addSignature(Signer* signer);

  //createPassThroughtReceipt
  static SharedPtr<ReturnReceipt> createPassThroughtReceipt(Node* node);

private:

  class Waiting;
  class ScopedSignature;

  CriticalSection                            lock;
  std::vector< SharedPtr<Waiting> >          waiting;
  std::set<Signer*>                          need_signature;
  std::vector< SharedPtr<ScopedSignature> >  scoped_signature;

};



///////////////////////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API DataflowValue
{
public:

  VISUS_CLASS(DataflowValue)

  //constructor
  DataflowValue() {
  }

  //destructor
  virtual ~DataflowValue() {
  }

  //wrapValue
  template <class Value>
  static SharedPtr<DataflowValue> wrapValue(Value value) {
    return wrapValue<Value>(value, is_shared_ptr<Value>{});
  }

  //unwrapValue
  template <class Value>
  static SharedPtr<Value> unwrapValue(SharedPtr<DataflowValue> obj);

private:

  template<class T>
  struct is_shared_ptr : std::false_type {};

  template<class T>
  struct is_shared_ptr< SharedPtr<T> > : std::true_type {};

  //wrapValue (for shared_ptr)
  template <class Value>
  static SharedPtr<DataflowValue> wrapValue(Value value, std::true_type);

  //wrapValue (for !shared_ptr)
  template <class Value>
  static SharedPtr<DataflowValue> wrapValue(Value value, std::false_type);


};


///////////////////////////////////////////////////////////////////////////////
template <class Value>
class WrappedDataflowValue : public DataflowValue
{
public:

  SharedPtr<Value> value;

  //constructor
  WrappedDataflowValue(SharedPtr<Value> value_ = SharedPtr<Value>()) : value(value_) {
  }

  //destructor
  virtual ~WrappedDataflowValue() {
  }

};


template <class Value>
inline SharedPtr<Value> DataflowValue::unwrapValue(SharedPtr<DataflowValue> obj) {
  auto wrapped = std::dynamic_pointer_cast< WrappedDataflowValue<Value> >(obj);
  return wrapped ? wrapped->value : SharedPtr<Value>();
}

template <class Value>
inline SharedPtr<DataflowValue> DataflowValue::wrapValue(Value value, std::true_type) {
  return std::make_shared< WrappedDataflowValue<typename Value::element_type> >(value);
}

template <class Value>
inline  SharedPtr<DataflowValue> DataflowValue::wrapValue(Value value, std::false_type) {
  return std::make_shared< WrappedDataflowValue<Value> >(std::make_shared<Value>(value));
}

///////////////////////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API DataflowMessage 
{
public:  

  VISUS_CLASS(DataflowMessage)

  typedef std::map<String, SharedPtr<DataflowValue> > Content;

  //constructor 
  DataflowMessage() : sender(nullptr) {
  }

  //destructor
  ~DataflowMessage() {
  }

  //getSender
  Node* getSender() const {
    return sender;
  }

  //setSender
  void setSender(Node* value) {
    this->sender=value;
  }

  //getReturnReceipt
  SharedPtr<ReturnReceipt> getReturnReceipt() const {
    return return_receipt;
  }

  //setReturnReceipt
  void setReturnReceipt(SharedPtr<ReturnReceipt> value) {
    this->return_receipt=value;
  }

  //getContent
  const Content& getContent() const {
    return content;
  }

  //hasContent
  bool hasContent(String key) const {
    return content.find(key)!=content.end();
  }

  //readValue
  SharedPtr<DataflowValue> readValue(String key) const {
    auto it = content.find(key); return (it != content.end()) ? it->second : SharedPtr<DataflowValue>();
  }

  //writeValue
  void writeValue(String key, SharedPtr<DataflowValue> value) {
    content[key] = value;
  }

  //readValue
  template <class Value>
  SharedPtr<Value> readValue(String key) {
    return DataflowValue::unwrapValue<Value>(readValue(key));
  }

  //writeValue
  template <class Value>
  void writeValue(String key, Value value) {
    writeValue(key, DataflowValue::wrapValue<Value>(value));
  }

private:

  Node*                        sender;
  Content                      content;
  SharedPtr<ReturnReceipt>     return_receipt;

};

} //namespace Visus

#endif //VISUS_DATAFLOW_MESSAGE_H__