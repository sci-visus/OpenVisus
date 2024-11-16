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

#ifndef VISUS_DATAFLOW_H__
#define VISUS_DATAFLOW_H__

#include <Visus/DataflowModule.h>
#include <Visus/Model.h>
#include <Visus/DataflowNode.h>
#include <Visus/ScopedVector.h>


namespace Visus {


  ////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API DataflowListener
{
public:

  VISUS_NON_COPYABLE_CLASS(DataflowListener)

  //constructor
  DataflowListener(){
  }

  //destructor
  virtual ~DataflowListener(){
  }

  //dataflowMessageHasBeenPublished
  virtual void dataflowMessageHasBeenPublished(DataflowMessage msg) {
  }

  //dataflowBeingDestroyed
  virtual void dataflowBeingDestroyed() {
  }

  //dataflowBeforeProcessInput
  virtual void dataflowBeforeProcessInput(Node* node) {
  }

  //dataflowBeforeProcessInput
  virtual void dataflowAfterProcessInput(Node* node) {
  }

  //dataflowSetNodeName
  virtual void dataflowSetNodeName(Node* node, String old_value, String new_value) {
  }

  //dataflowSetNodeVisible
  virtual void dataflowSetNodeVisible(Node* node, bool old_value, bool new_value) {
  }

  //dataflowAddNode
  virtual void dataflowAddNode(Node* node) {
  }

  //dataflowRemoveNode
  virtual void dataflowRemoveNode(Node* node) {
  }

  //dataflowMoveNode
  virtual void dataflowMoveNode(Node* dst, Node* src, int index) {
  }

  //dataflowSetSelection
  virtual void dataflowSetSelection(Node* old_selection, Node* new_selection) {
  }

  //datafloConnectNodes
  virtual void dataflowConnectNodes(Node* from, String oport, String iport, Node* to) {
  }

  //datafloConnectNodes
  virtual void dataflowDisconnectNodes(Node* from, String oport, String iport, Node* to) {
  }

};

////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API Dataflow 
{
public:

  VISUS_NON_COPYABLE_CLASS(Dataflow)

  typedef DataflowListener Listener;

  std::vector<Listener*> listeners;

  //constructor
  Dataflow();

  //destructor
  virtual ~Dataflow();

  //addListener
  void addListener(Listener* value) {
    listeners.push_back(value);
  }

  //removeListener
  void removeListener(Listener* value) {
    Utils::remove(listeners, value);
  }

  //guessNodeUIID
  String guessNodeUIID(String base)
  {
    VisusReleaseAssert(!base.empty());

    if (!findNodeByUUID(base))
      return base;

    for (int I=1;;I++)
    {
      auto maybe = base + cstring(I);
      if (!findNodeByUUID(maybe))
        return maybe;
    }
    
    return "";
  }

  //getRoot
  Node* getRoot() const {
    return nodes.empty() ? nullptr : const_cast<Node*>(nodes[0]);
  }

  //getNodes
  const ScopedVector<Node>& getNodes() const{
    return nodes;
  }

  //getNodesAsVector (needed for swig)
  std::vector<Node*> getNodesAsVector() const {
    std::vector<Node*> ret;
    for (auto node : getNodes())
      ret.push_back(node);
    return ret;
  }

  //findNodeByUUID
  Node* findNodeByUUID(const String& uuid) const
  {
    if (uuid.empty()) return nullptr;
    auto it = uuids.find(uuid);
    return it == uuids.end() ? nullptr : it->second;
  }


  //processInput
  virtual void processInput(Node* node);

  //needProcessInput
  void needProcessInput(Node* node) {
    VisusAssert(VisusHasMessageLock());;
    need_processing.insert(node);  
  }

  //publish
  bool publish(DataflowMessage msg);

  //abortProcessing
  void abortProcessing();

  //joinProcessing
  void joinProcessing();

  //guessLastPublished
  DataflowPortValue* guessLastPublished(DataflowPort* from);

  //dispatchPublishedMessages
  bool dispatchPublishedMessages();

  //containsNode
  bool containsNode(Node* node) const{
    return node && node->getDataflow() == this;
  }

  //selection
  Node* getSelection() const{
    return selection;
  }

  ///setSelection
  void setSelection(Node* value);

  //dropSelection
  void dropSelection(){
    setSelection(nullptr);
  }

  //addNode
  void addNode(Node* parent, Node* VISUS_DISOWN(node), int index = -1);

  //addNode
  void addNode(Node* VISUS_DISOWN(node)){
    addNode(nullptr, node);
  }

  //canMoveNode
  bool canMoveNode(Node* dst,Node* src);

  //moveNode
  void moveNode(Node* dst, Node* src, int index = -1);

  //removeNode
  void removeNode(Node* node);

  //connectNodes
  void connectNodes(Node* from, String oport, String iport, Node* to);

  //connectNodes
  void connectNodes(Node* from, String port_name, Node* to) {
    connectNodes(from, port_name, port_name, to);
  }

  //connectNodes
  void connectNodes(Node* from, Node* to)
  {
    std::vector<String> common;
    for (auto oport : from->getOutputPortNames())
    {
      if (to->hasInputPort(oport))
        common.push_back(oport);
    }
    if (common.size() != 1)
      ThrowException("internal error");
    return connectNodes(from, common[0], to);
  }

  //disconnectNodes
  void disconnectNodes(Node* from, String oport, String iport, Node* to);

public:

  //write
  void write(Archive& ar) const;

  //read
  void read(Archive& ar) ;

private:

  std::map<String, Node*>       uuids;
  ScopedVector<Node>            nodes;
  Node*                         selection = nullptr;

  //use this variable only from the main thread.... I'm not using any lock for this!
  std::set<Node*>               need_processing;

  //runtime
  CriticalSection               published_lock;
  std::vector<DataflowMessage>  published;

  //floodValueForward
  void floodValueForward(DataflowPort* port, SharedPtr<DataflowValue> value, const SharedPtr<ReturnReceipt>& return_receipt);

};


} //namespace Visus

#endif //VISUS_DATAFLOW_H__

