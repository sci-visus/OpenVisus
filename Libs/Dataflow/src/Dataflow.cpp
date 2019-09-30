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

namespace Visus {

//////////////////////////////////////////////////////////
Dataflow::Dataflow() 
{
}

//////////////////////////////////////////////////////////
Dataflow::~Dataflow()
{
  VisusAssert(VisusHasMessageLock());

  abortProcessing();
  joinProcessing();

  while (!listeners.empty())
    listeners[0]->dataflowBeingDestroyed();

  {
    ScopedLock lock(this->published_lock);
    this->published.clear();
  }
  need_processing.clear();

  //in a multiple dataflow is important at least to disconnect the ports
  for (int I=0;I<(int)nodes.size();I++)
  {
    Node* node=nodes[I];
    node->exitFromDataflow();
    for (auto it=node->inputs .begin();it!=node->inputs .end();it++) it->second->disconnect();
    for (auto it=node->outputs.begin();it!=node->outputs.end();it++) it->second->disconnect();
    node->dataflow=nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////
void Dataflow::processInput(Node* node)
{
  if (!node->isVisible())
    return;

  for (auto it : listeners)
    it->dataflowBeforeProcessInput(node);
  
  node->processInput();

  for (auto it : listeners)
    it->dataflowAfterProcessInput(node);
}

//////////////////////////////////////////////////////////
void Dataflow::abortProcessing()
{
  VisusAssert(VisusHasMessageLock());

  for (int I=0;I<nodes.size();I++)
    nodes[I]->abortProcessing();
}


//////////////////////////////////////////////////////////
void Dataflow::joinProcessing()
{
  VisusAssert(VisusHasMessageLock());
  for (int I=0;I<nodes.size();I++)
    nodes[I]->joinProcessing();
}

////////////////////////////////////////////////////////////////////
void Dataflow::floodValueForward(DataflowPort* port,SharedPtr<DataflowValue> value,const SharedPtr<ReturnReceipt>& return_receipt)
{
  VisusAssert(VisusHasMessageLock());
  Node* node=port->getNode();
  VisusAssert(containsNode(node));
  port->writeValue(value,return_receipt);

  if (node->getInputPort(port->getName())==port)
    need_processing.insert(node);

  for (auto it=port->outputs.begin();it!=port->outputs.end();it++)
    floodValueForward((*it),value,return_receipt);
}

////////////////////////////////////////////////////////////////////////
/* 
case OPORT_IPORT

      [----------dataflow==DATAFLOW----------]
      [                                      ]
      [   [------]                [------]   ]
      [   [ node ]::from ---> to::[ NODE ]   ]
      [   [------]                [ -----]   ]
      [                                      ]
      [--------------------------------------]
  
case IPORT_OPORT

      [----------dataflow==DATAFLOW--------]
      [                                    ]
      [         [------------]             ]
      [   from::[ node==NODE ]::to         ]
      [         [------------]             ]
      [                                    ]
      [------------------------------------]
  
case OPORT_OPORT

      [-----------DATAFLOW-------------------]
      [                                      ]
      [    [------NODE==dataflow----]        ]
      [    [                        ]        ]
      [    [        [node]::from--->]::to    ]
      [    [                        ]        ] 
      [    [------------------------]        ]
      [                                      ]
      [--------------------------------------]
  
case IPORT_IPORT

      [-----------dataflow-------------------]
      [                                      ]
      [          [------node==DATAFLOW----]  ]
      [          [                        ]  ]
      [    from::[   to::[NODE]           ]  ]
      [          [                        ]  ] 
      [          [------------------------]  ]
      [                                      ]
      [--------------------------------------]
*/
////////////////////////////////////////////////////////////////////////


DataflowPortValue* Dataflow::guessLastPublished(DataflowPort* from)
{
  VisusAssert(VisusHasMessageLock());

  //this is the simple algorithm which covers 99% of the cases (mostly when I don't have multiple dataflow)
  if (DataflowPortValue* ret=from->previewValue())
    return ret;

  //going backward
  {
    std::deque<DataflowPort*> backward;

    //no ambiguity
    if (from->inputs.size()==1) 
      backward.push_back(*from->inputs.begin()); 

    while (!backward.empty())
    {
      if (DataflowPortValue* ret=backward.front()->previewValue())
        return ret;

      //no ambiguity
      if (backward.front()->inputs.size()==1) 
        backward.push_back(*backward.front()->inputs.begin());

      backward.pop_front();
    }

  }

  //going forward
  {
    std::deque<DataflowPort*> forward;

    for (auto it=from->outputs.begin();it!=from->outputs.end();it++)
      {if ((*it)->inputs.size()==1) forward.push_back(*it);}

    while (!forward.empty())
    {
      if (DataflowPortValue* ret=forward.front()->previewValue())
        return ret;

      for (auto it=forward.front()->outputs.begin();it!=forward.front()->outputs.end();it++)
        {if ((*it)->inputs.size()==1) forward.push_back(*it);}

      forward.pop_front();
    }
  }

  return nullptr; //not found
}


////////////////////////////////////////////////////////////////////////////////////////////////
bool Dataflow::dispatchPublishedMessages()
{
  VisusAssert(VisusHasMessageLock());

  //make this very fast
  std::vector<DataflowMessage> published;
  {
    ScopedLock lock(this->published_lock); //need the lock

    //no need to do anything
    if (this->published.empty() && need_processing.empty())
      return false;

    std::swap(published,this->published);
  }
  //floodValues stored in the publish event
  for (auto& msg : published)
  {
    //probably the node has been removed from the dataflow (see removeNode)
    if (Node* sender=msg.getSender())
    {
      VisusAssert(containsNode(sender));
      for (auto it=msg.getContent().begin();it!=msg.getContent().end();it++)
      {
        String port_name=it->first;

        SharedPtr<DataflowValue> value_to_flood=it->second;
        DataflowPort* port=sender->getOutputPort(port_name);

        //make sure it's not removed meanwhile
        if (!port || !port->getNode() || !port->getNode()->getDataflow())
          continue;

        floodValueForward(port,value_to_flood,msg.getReturnReceipt());
      }

      sender->messageHasBeenPublished(msg);
    }

    for (auto listener : listeners)
      listener->dataflowMessageHasBeenPublished(msg);

    //I promised to sign it in Dataflow::publish
    if (auto return_receipt=msg.getReturnReceipt())
      return_receipt->addSignature(this);
  }

  //now I forward the input changes 
  for (auto node : need_processing)
  {
    VisusAssert(containsNode(node));

    Dataflow* dataflow=node->getDataflow();
    if (!dataflow) continue;
    processInput(node);
  }
  need_processing.clear();
  return true;
}

//////////////////////////////////////////////////////////
bool Dataflow::publish(DataflowMessage msg)
{
  //I need the lock, I can be in any thread here
  {
    ScopedLock lock(published_lock);
    published.push_back(msg);

    if (auto return_receipt = msg.getReturnReceipt())
      return_receipt->needSignature(this);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////
void Dataflow::setSelection(Node* node)
{
  VisusAssert(VisusHasMessageLock());

  auto old=getSelection();
  if (old==node) 
    return;

  this->selection=node;

  for (auto listener : listeners)
    listener->dataflowSetSelection(old,node);

}


//////////////////////////////////////////////////////////
void Dataflow::addNode(Node* parent,Node* node,int index)
{
  VisusAssert(VisusHasMessageLock());

  VisusAssert(node && !node->dataflow && node->childs.empty() && !node->parent);

  if (selection)
    dropSelection();

  this->nodes.push_back(node);
  node->dataflow=this;
  String uuid=node->uuid;
  VisusAssert(uuids.find(uuid)==uuids.end());
  uuids[uuid]=node;

  node->enterInDataflow();

  //parent (if specified) must be already in the tree
  if (parent)
  {
    VisusAssert(parent==getRoot() || parent->parent!=nullptr); 
    parent->addChild(node,index);
  }

  for (auto listener : listeners)
    listener->dataflowAddNode(node);
}


//////////////////////////////////////////////////////////
void Dataflow::removeNode(Node* NODE)
{
  VisusAssert(VisusHasMessageLock());
  VisusAssert(NODE && containsNode(NODE));

  abortProcessing();
  joinProcessing();

  if (selection)
    dropSelection();

  //remove all bottom->up
  for (auto node : NODE->reversedBreadthFirstSearch())
  {
    VisusAssert(node->getDataflow()==this);

    for (auto listener : listeners)
      listener->dataflowRemoveNode(node);

    for (auto input : node->inputs) 
      input.second->disconnect();

    for (auto output : node->outputs)
      output.second->disconnect();

    VisusAssert(node->isOrphan());
    VisusAssert(node->childs.empty());

    String uuid=node->uuid;
    VisusAssert(!uuid.empty() && uuids.find(uuid)!=uuids.end() && uuids[uuid]==node);
    node->exitFromDataflow();

    uuids.erase(uuid);

    if (node->parent)
    {
      node->parent->removeChild(node);
      node->parent=nullptr;
    }

    //invalidate published message 
    {
      ScopedLock lock(published_lock);
      for (auto& msg : this->published)
      {
        if (msg.getSender()==node)
          msg.setSender(nullptr);
      }
    }

    this->need_processing.erase(node);

    node->dataflow=nullptr;
    this->nodes.erase(this->nodes.find(node));
  }
}

////////////////////////////////////////////////////////////////////////////
bool Dataflow::canMoveNode(Node* dst,Node* src)
{
  if (!dst || !src) 
    return false;

  //cannot move root
  if (!src->getParent())
    return false;

  //<dst> cannot be <src> or any child of <src> 
  auto bfs=src->breadthFirstSearch();
  if (std::find(bfs.begin(),bfs.end(),dst)!=bfs.end())
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////////
void Dataflow::moveNode(Node* dst,Node* src,int index)
{
  VisusAssert(VisusHasMessageLock());

  if (!canMoveNode(dst,src))
    return;

  src->getParent()->removeChild(src);
  dst->addChild(src,index);

  for (auto listener : listeners)
    listener->dataflowMoveNode(dst,src,index);
}

////////////////////////////////////////////////////////////////////////
void Dataflow::connectPorts(Node* from,String oport_name,String iport_name,Node* to)
{
  VisusAssert(VisusHasMessageLock());

  DataflowPort* oport=from->getOutputPort(oport_name);
  DataflowPort* iport=to  ->getInputPort(iport_name);

  VisusAssert(iport && containsNode(from) && iport->inputs .find(oport)==iport->inputs .end());
  VisusAssert(oport && containsNode(to  ) && oport->outputs.find(iport)==oport->outputs.end());

  DataflowPortValue* last_published=guessLastPublished(oport);

  oport->outputs.insert(iport);
  iport->inputs .insert(oport);

  for (auto listener : listeners)
    listener->dataflowConnectPorts(from,oport_name,iport_name,to);

  if (last_published)
    floodValueForward(iport,last_published->value,last_published->return_receipt);
}

////////////////////////////////////////////////////////////////////////
void Dataflow::disconnectPorts(Node* from,String oport_name,String iport_name,Node* to)
{
  DataflowPort* oport=from->getOutputPort(oport_name);
  DataflowPort* iport=to  ->getInputPort (iport_name);

  VisusAssert(VisusHasMessageLock());
  VisusAssert(oport && containsNode(from) && oport->outputs.find(iport)!=oport->outputs.end());
  VisusAssert(iport && containsNode(to  ) && iport->inputs .find(oport)!=iport->inputs.end());

  oport->outputs.erase(iport);
  iport->inputs .erase(oport);

  for (auto listener : listeners)
    listener->dataflowDisconnectPorts(from,oport_name,iport_name,to);
}


/////////////////////////////////////////////////////////
void Dataflow::writeTo(StringTree& out) const
{
  VisusAssert(false);
}


/////////////////////////////////////////////////////////
void Dataflow::readFrom(StringTree& in)
{
  VisusAssert(false);
}

} //namespace Visus 
