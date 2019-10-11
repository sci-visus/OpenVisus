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

#include <Visus/Dataflow.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
#include <Visus/ApplicationInfo.h>

namespace Visus {

////////////////////////////////////////////////////////////
Node::Node(String name) :  name(name),dataflow(nullptr),visible(true),parent(nullptr),uuid("")
{
}

////////////////////////////////////////////////////////////
Node::~Node()
{
  VisusAssert(!dataflow);
  for (auto it=inputs .begin();it!=inputs .end();it++) delete it->second;
  for (auto it=outputs.begin();it!=outputs.end();it++) delete it->second;
}

////////////////////////////////////////////////////////////
String Node::getTypeName() const
{
  return NodeFactory::getSingleton()->getTypeName(*this);
}

////////////////////////////////////////////////////////////
void Node::execute(Archive& ar) {
  
  if (ar.name == "set")
  {
    String target_id;
    ar.read("target_id", target_id);

    if (target_id == "name")
    {
      String value;
      ar.read("value", value);
      setName(value);
      return;
    }

    if (target_id == "visible")
    {
      bool value=true;
      ar.read("value", value);
      setVisible(value);
      return;
    }
  }
  
  return Model::execute(ar);
}


////////////////////////////////////////////////////////////////////////////
void Node::addChild(Node* child,int index)
{
  VisusAssert(child->parent==nullptr);
  if (index<0 || index>(int)childs.size())
    childs.push_back(child);
  else
    childs.insert(childs.begin()+index,child);
  child->parent=this;
}


////////////////////////////////////////////////////////////////////////////
void Node::removeChild(Node* child)
{
  VisusAssert(child->parent==this);
  childs.erase(std::remove(childs.begin(), childs.end(), child), childs.end());
  child->parent=nullptr;
}


//////////////////////////////////////////////////////////
void Node::setName(String new_value)
{
  VisusAssert(VisusHasMessageLock());
  VisusAssert(!new_value.empty());

  auto old_value=getName();
  if (old_value==new_value)
    return;

  setProperty("name", this->name, new_value);

  if (dataflow)
  {
    for (auto listener : dataflow->listeners)
      listener->dataflowSetName(this,old_value,new_value);
  }
}

//////////////////////////////////////////////////////////
void Node::setVisible(bool new_value)
{
  VisusAssert(VisusHasMessageLock());

  auto old_value=isVisible();
  if (old_value==new_value)
    return;

  setProperty("visible", this->visible, new_value);

  if (dataflow)
  {
    for (auto listener : dataflow->listeners)
      listener->dataflowSetHidden(this,old_value,new_value);
  }
}

////////////////////////////////////////////////////////////////////////////
std::vector<Node*> Node::getPathToRoot() const
{
  std::vector<Node*> ret;
  for (Node* cursor=const_cast<Node*>(this);cursor;cursor=cursor->parent) 
    ret.push_back(cursor);
  return ret;
}


////////////////////////////////////////////////////////////////////////////
std::vector<Node*> Node::getPathFromRoot() const
{
  std::vector<Node*> ret=getPathToRoot();
  std::reverse(ret.begin(),ret.end());
  return ret;
}


////////////////////////////////////////////////////////////////////////////
int Node::getIndexInParent() const
{
  if (!this->parent) return -1;
  const std::vector<Node*>& v=this->parent->childs;
  return (int)std::distance(v.begin(),std::find(v.begin(),v.end(),this));
}


////////////////////////////////////////////////////////////////////////////
Node* Node::goUpIncludingBrothers() const
{
  if (!this->parent) return nullptr;
  int N=getIndexInParent();
  return N>0? this->parent->childs[N-1] : this->parent;
}

////////////////////////////////////////////////////////////////////////////
std::vector<Node*> Node::breadthFirstSearch() const
{
  std::vector<Node*> ret;
  std::deque<const Node*>  traversal;
  traversal.push_back(this);
  while (!traversal.empty())
  {
    auto cursor=traversal.front();
    traversal.pop_front();

    ret.push_back(const_cast<Node*>(cursor));
    for (auto child : cursor->childs)
      traversal.push_back(child);
  }
  return ret;
}


////////////////////////////////////////////////////////////////////////////
Node* Node::findChildWithName(String name) const
{
  for (auto child : childs)
  {
    if (child->name==name)
      return child;
  }
  return nullptr;
}


////////////////////////////////////////////////////////////////////////////
String Node::guessUniqueChildName(String prefix) const
{
  for (int I=1;;I++)
  {
    String ret=prefix + " " + cstring(I);
    if (!findChildWithName(ret)) 
      return ret;
  }
  VisusAssert(false);
  return "";
}

////////////////////////////////////////////////////////////
void Node::enterInDataflow()
{
  VisusAssert(VisusHasMessageLock());
}

////////////////////////////////////////////////////////////
void Node::exitFromDataflow()
{
  VisusAssert(VisusHasMessageLock());

  abortProcessing();
  joinProcessing();
}

////////////////////////////////////////////////////////////
void Node::abortProcessing()
{
  VisusAssert(VisusHasMessageLock());

  {
    ScopedLock lock(running_lock);
    for (auto it : running)
      it->abort();
  }
}


////////////////////////////////////////////////////////////
void Node::joinProcessing()
{
  VisusAssert(VisusHasMessageLock());

  if (thread_pool)
    thread_pool->waitAll();
}

////////////////////////////////////////////////////////////
void Node::addNodeJob(SharedPtr<NodeJob> job)
{
  VisusAssert(VisusHasMessageLock());
  VisusAssert(job && getDataflow()!=nullptr);

  //create a thread for the jobs
  if (!thread_pool)
    thread_pool=std::make_shared<ThreadPool>(name + " " + "Worker",1);

  {
    ScopedLock lock(running_lock);
    running.insert(job);

    job->done.when_ready([this,job](int nworker) {
      
      ScopedLock lock(running_lock);
      running.erase(job);
    });
  }

  ThreadPool::push(thread_pool,[job]()
  {
    if (!job->aborted())
      job->runJob();

    job->done.set_value(1);
  });
}


////////////////////////////////////////////////////////////
bool Node::addInputPort(String name,int policy)
{
  VisusAssert(VisusHasMessageLock());
  if (name.empty() || hasInputPort(name)) return false;
  DataflowPort* port=new DataflowPort();
  port->setNode(this);
  port->setName(name);
  port->setPolicy(policy);
  this->inputs[name]=port;
  return true;
}

////////////////////////////////////////////////////////////
bool Node::addOutputPort(String name,int policy)
{
  VisusAssert(VisusHasMessageLock());
  if (name.empty() || hasOutputPort(name)) return false;
  DataflowPort* port=new DataflowPort();
  port->setNode(this);
  port->setName(name);
  port->setPolicy(policy);
  this->outputs[name]=port;
  return true;
}

////////////////////////////////////////////////////////////
bool Node::removeInputPort(String name)
{
  VisusAssert(VisusHasMessageLock());
  DataflowPort* port=getInputPort(name);
  VisusAssert(port && port->getNode()==this);
  port->disconnect();
  this->inputs.erase(port->getName());
  delete port;
  return true;
}

////////////////////////////////////////////////////////////
bool Node::removeOutputPort(String name)
{
  VisusAssert(VisusHasMessageLock());
  DataflowPort* port=getOutputPort(name);
  VisusAssert(port && port->getNode()==this);
  port->disconnect();
  this->outputs.erase(port->getName());
  delete port;
  return true;
}

////////////////////////////////////////////////////////////
DataflowPort* Node::getInputPort(String iport) const
{
  VisusAssert(VisusHasMessageLock());
  auto it=this->inputs.find(iport);
  return it==this->inputs.end()? nullptr : it->second;
}

////////////////////////////////////////////////////////////
DataflowPort* Node::getOutputPort(String oport) const
{
  VisusAssert(VisusHasMessageLock());
  auto it=this->outputs.find(oport);
  return it==this->outputs.end()? nullptr : it->second;
}


////////////////////////////////////////////////////////////
std::vector<String> Node::getInputPortNames() const
{
  VisusAssert(VisusHasMessageLock());
  std::vector<String> ret;ret.reserve(this->inputs.size());
  for (auto it=this->inputs.begin();it!=this->inputs.end();it++)
    ret.push_back(it->first);
  return ret;
}

////////////////////////////////////////////////////////////
std::vector<String> Node::getOutputPortNames() const
{
  VisusAssert(VisusHasMessageLock());
  std::vector<String> ret;ret.reserve(this->outputs.size());
  for (auto it=this->outputs.begin();it!=this->outputs.end();it++)
    ret.push_back(it->first);
  return ret;
}

////////////////////////////////////////////////////////////
bool Node::isInputConnected(String iport) const
{
  VisusAssert(VisusHasMessageLock());
  DataflowPort* port=this->getInputPort(iport);
  return port &&  !port->inputs.empty();
}

////////////////////////////////////////////////////////////
bool Node::isOutputConnected(String oport) const
{
  VisusAssert(VisusHasMessageLock());
  DataflowPort* port=this->getOutputPort(oport);
  return port &&  !port->outputs.empty();
}

////////////////////////////////////////////////////////////
bool Node::isOrphan() const
{
  for (auto it=inputs.begin();it!=inputs.end();it++)
    if (!it->second->inputs.empty() /*&& it->second->outputs.empty()*/) return false;

  for (auto it=outputs.begin();it!=outputs.end();it++)
    if (!it->second->outputs.empty() /*&& it->second->inputs.empty()*/) return false;

  return true;
}

////////////////////////////////////////////////////////////
bool Node::needProcessInputs() const
{
  VisusAssert(VisusHasMessageLock());
  for (auto it=inputs.begin();it!=inputs.end();it++)
    if (it->second->hasNewValue()) return true;
  return false;
}

////////////////////////////////////////////////////////////
SharedPtr<DataflowValue> Node::readValue(String iport)
{
  VisusAssert(VisusHasMessageLock());
  DataflowPort* port=this->getInputPort(iport);
  if (!port) return SharedPtr<DataflowValue>();
  return port->readValue();
}

////////////////////////////////////////////////////////////
SharedPtr<DataflowValue> Node::previewInput(String iport)
{
  VisusAssert(VisusHasMessageLock());
  DataflowPort* port=this->getInputPort(iport);
  if (!port) return SharedPtr<DataflowValue>();

  if (auto preview_value=port->previewValue())
    return preview_value->value;

  return SharedPtr<DataflowValue>();
}

////////////////////////////////////////////////////////////
void Node::write(Archive& ar) const
{
  ar.write("uuid", uuid);
  ar.write("name", name);
  ar.write("visible", visible);
}

////////////////////////////////////////////////////////////
void Node::read(Archive& ar)
{
  this->visible = true;
  ar.read("uuid", uuid);
  ar.read("name", name);
  ar.read("visible", visible);
}

//////////////////////////////////////////////////////////
SharedPtr<ReturnReceipt> Node::createPassThroughtReceipt()
{
  return ReturnReceipt::createPassThroughtReceipt(this);
}

//////////////////////////////////////////////////////////
bool Node::publish(DataflowMessage msg)
{
  msg.setSender(this);

  if (!dataflow)
    return false;

  //ABSOLUTELY HERE DO NOT READ ANYTHING RELATED to the node connectivity, you can be in a different thread!
  //redirect public message to Dataflow, the call can come from a different Thread
  //so I need to be sure that "dataflow" variable is stable

  return dataflow->publish(msg);
}

} //namespace Visus 

