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

#ifndef VISUS_DATAFLOW_NODE_H__
#define VISUS_DATAFLOW_NODE_H__

#include <Visus/DataflowModule.h>

#include <Visus/Model.h>
#include <Visus/ThreadPool.h>
#include <Visus/Rectangle.h>
#include <Visus/DataflowPort.h>
#include <Visus/Position.h>
#include <Visus/Async.h>
#include <Visus/Array.h>

namespace Visus {

//predeclaration
class Dataflow;

/////////////////////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API NodeJob 
{
public:

  VISUS_CLASS(NodeJob)

  //aborted
  Aborted aborted;

  //done
  Promise<int> done;

  //constructor
  NodeJob() {
  }

  //destructor
  virtual ~NodeJob() {
  }

  //pure virtual runJob
  virtual void runJob() = 0;


  //abort
  virtual void abort() {
    this->aborted.setTrue();
  }


};

////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API Node : public Model
{
public:

  VISUS_NON_COPYABLE_CLASS(Node)

  //input/outputs
  std::map<String,DataflowPort*> outputs;
  std::map<String,DataflowPort*> inputs;

  //see DataflowFrameView
  Rectangle2d frameview_bounds;

  //constructor
  Node(String name="");

  //destructor (please remove from the dataflow before destroying!)
  virtual ~Node();

  //getTypeName
  virtual String getTypeName() const override;

  //getOsDependentTypeName
  virtual String getOsDependentTypeName() const {
    return typeid(*this).name();
  }

  //getName
  String getName() const {
    return name;
  }

  //setName
  void setName(String value);

  //getUUID
  String getUUID() const {
    return uuid;
  }

  //getParent
  Node* getParent() {
    return parent;
  }

  //isHidden
  bool isHidden() const {
    return hidden;
  }

  //setHidden
  void setHidden(bool value);

  //getChilds
  const std::vector<Node*>& getChilds()  const {
    return childs;
  }

  //getNodeBounds
  virtual Position getNodeBounds() {
    return Position::invalid();
  }

  //getDataflow
  Dataflow* getDataflow() const
  {return this->dataflow;}

  //enterInDataflow
  virtual void enterInDataflow();

  //exitFromDataflow
  virtual void exitFromDataflow();

  //addNodeJob
  virtual void addNodeJob(SharedPtr<NodeJob> job);

  //abortProcessing
  virtual void abortProcessing();

  //joinProcessing 
  virtual void joinProcessing();

  // ********************************************************
  // IMPORTANT the following function are not thread safe.... 
  // make sure you use them only in the main thread
  // *********************************************************

  //addInputPort
  bool addInputPort(String name,int policy=DataflowPort::DefaultInputPortPolicy);

  //addOutputPort
  bool addOutputPort(String name,int policy=DataflowPort::DefaultOutputPortPolicy);

  //getInputPort
  DataflowPort* getInputPort(String iport) const;

  //getOutputPort
  DataflowPort* getOutputPort(String oport) const;

  //removeInputPort
  bool removeInputPort(String name);

  //removeOutputPort
  bool removeOutputPort(String name);

  //hasInputPort
  bool hasInputPort(String iport)  const {
    return getInputPort(iport) ? true : false;
  }

  //hasOutputPort
  bool hasOutputPort(String oport)  const {
    return getOutputPort(oport) ? true : false;
  }

  //getInputPortNames
  std::vector<String> getInputPortNames() const;

  //getOutputPortNames
  std::vector<String> getOutputPortNames() const;

  //isInputConnected
  bool isInputConnected(String iport) const;

  //isOutputConnected
  bool isOutputConnected(String oport) const;

  //isOrphan
  bool isOrphan() const;

  //needProcessInputs
  bool needProcessInputs() const;

  //readValue
  SharedPtr<DataflowValue> readValue(String iport);

  //readValue
  template <class Value>
  SharedPtr<Value> readValue(String iport) {
    return DataflowValue::unwrapValue<Value>(readValue(iport));
  }

  //previewInput
  SharedPtr<DataflowValue> previewInput(String iport);

  //publish
  bool publish(DataflowMessage msg);

  //in case you need to know when published message is dispatched
  virtual void messageHasBeenPublished(DataflowMessage msg){
    VisusAssert(VisusHasMessageLock());
  }

  //getFirstInputPort
  DataflowPort* getFirstInputPort() const{
    return inputs.empty() ? nullptr : inputs.begin()->second;
  }

  //getFirstOutputPort
  DataflowPort* getFirstOutputPort() const{
    return outputs.empty() ? nullptr : outputs.begin()->second;
  }

  //getPathToRoot
  std::vector<Node*> getPathToRoot() const;

  //getPathFromRoot
  std::vector<Node*> getPathFromRoot() const;

  //getIndexInParent
  int getIndexInParent() const;

  //goUpIncludingBrothers
  Node* goUpIncludingBrothers() const;

  //findChild
  template <class Type>
  Type findChild(bool bRecursive=false) const
  {
    for (auto child : bRecursive? breadthFirstSearch() : this->childs){
      if (Type ret=dynamic_cast<Type>(child)) 
        return ret;
    }
    return nullptr;
  }

  //breadthFirstSearch
  std::vector<Node*> breadthFirstSearch() const;

  //reversedBreadthFirstSearch
  std::vector<Node*> reversedBreadthFirstSearch() const {
    auto ret = breadthFirstSearch();
    std::reverse(ret.begin(), ret.end());
    return ret;
  }


  //findChildWithName
  Node* findChildWithName(String name) const;

  //guessUniqueChildName
  String guessUniqueChildName(String prefix) const;

  //createPassThroughtReceipt
  SharedPtr<ReturnReceipt> createPassThroughtReceipt();

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

protected:

  friend class Dataflow;

  Dataflow*                    dataflow;
  String                       uuid;
  String                       name;
  bool                         hidden;
  Node*                        parent;
  std::vector<Node*>           childs;

  CriticalSection                running_lock;
  std::set< SharedPtr<NodeJob> > running;

  SharedPtr<ThreadPool>          thread_pool;

  //processInput 
  virtual bool processInput() {
    return false;
  }

  //addChild
  void addChild(Node* child,int index=-1);

  //removeChild
  void removeChild(Node* child);

};


///////////////////////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API NodeCreator
{
public:

  VISUS_CLASS(NodeCreator)

  //constructor
  NodeCreator() {
  }

  //destructor
  virtual ~NodeCreator() {
  }

  //createInstance
  virtual VISUS_NEWOBJECT(Node*) createInstance() {
    ThrowException("internal error, you forgot to implement createInstance");
    return nullptr;
  }
};


///////////////////////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API NodeFactory
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(NodeFactory)

  //desgtructor
  ~NodeFactory() {
    for (auto it : creators)
      delete it.second;
  }

  //registerClass
  void registerClass(String portable_typename, String os_typename, NodeCreator* VISUS_DISOWN(creator)) {
    VisusAssert(creators.find(portable_typename) == creators.end());
    creators[portable_typename] = creator;
    portable_typenames.setValue(os_typename, portable_typename);
  }

  //registerClass
  template <typename NodeClass>
  void registerClass(String portable_typename) {
    return registerClass(portable_typename, typeid(NodeClass).name(), new CppNodeCreator<NodeClass>());
  }

  //createInstance
  VISUS_NEWOBJECT(Node*) createInstance(String portable_typename, bool bCanFail = false)
  {
    auto it = creators.find(portable_typename);
    if (it == creators.end()) { VisusAssert(bCanFail); return nullptr; }
    return it->second->createInstance();
  }

  //createInstance
  template <class NodeClass>
  VISUS_NEWOBJECT(NodeClass*) createInstance(String portable_typename, bool bCanFail = false)
  {
    Node* obj = createInstance(portable_typename, bCanFail);
    if (NodeClass* ret = dynamic_cast<NodeClass*>(obj)) return ret;
    if (obj) delete obj;
    return nullptr;
  }

  //getTypeName
  String getTypeName(const Node& instance)
  {
    String os_dependent_typename = instance.getOsDependentTypeName();
    VisusAssert(portable_typenames.hasValue(os_dependent_typename));
    return portable_typenames.getValue(os_dependent_typename);
  }

private:

  //constructor
  NodeFactory() {
  }

  std::map<String, NodeCreator*> creators;
  StringMap                      portable_typenames;


  //_______________________________________________
  template <class NodeClass>
  class CppNodeCreator : public NodeCreator
  {
  public:

    //constructor
    CppNodeCreator() {
    }

    //destructor
    virtual ~CppNodeCreator() {
    }

    //createInstance
    virtual VISUS_NEWOBJECT(Node*) createInstance() override {
      return new NodeClass();
    }

  private:

    VISUS_NON_COPYABLE_CLASS(CppNodeCreator)
  };

};


//VISUS_REGISTER_NODE_CLASS
#define VISUS_REGISTER_NODE_CLASS(NodeClass) \
    Visus::NodeFactory::getSingleton()->registerClass<NodeClass>(#NodeClass) \
    /*--*/

} //namespace Visus

#endif //VISUS_DATAFLOW_NODE_H__
