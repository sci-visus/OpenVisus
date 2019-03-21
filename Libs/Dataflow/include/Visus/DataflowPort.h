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

#ifndef VISUS_DATAFLOW_PORT_H__
#define VISUS_DATAFLOW_PORT_H__

#include <Visus/DataflowModule.h>
#include <Visus/DataflowMessage.h>
#include <Visus/Time.h>

#include <deque>

namespace Visus {

//predeclaration
class Node;

////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API DataflowPortValue
{
public:

  VISUS_CLASS(DataflowPortValue)

  SharedPtr<DataflowValue>   value;
  int                        write_id=0;
  Int64                      write_timestamp = 0;
  SharedPtr<ReturnReceipt>   return_receipt;

  //constructor
  DataflowPortValue() {
  }
};

////////////////////////////////////////////////////////////
class VISUS_DATAFLOW_API DataflowPort 
{
public:

  VISUS_NON_COPYABLE_CLASS(DataflowPort)

  enum Policy
  {
    DoNotStoreValue             = 0, 
    StoreOnlyOnePersistentValue = 1, 
    StoreOnlyOneVolatileValue   = 2,
    StoreMultipleVolatileValues = 3,

    DefaultInputPortPolicy =StoreOnlyOnePersistentValue,
    DefaultOutputPortPolicy=DoNotStoreValue
  };

  typedef std::set<DataflowPort*>::iterator iterator;

  std::set<DataflowPort*> inputs;
  std::set<DataflowPort*> outputs;

  //constructor
  inline DataflowPort() : policy(DoNotStoreValue), node(0),write_id(0),read_id(0)
  {}

  //destructor
  virtual ~DataflowPort()
  {}

  //getNode
  inline Node* getNode()
  {return node;}

  //ode(Da
  inline void setNode(Node* node)
  {this->node=node;}

  //getName
  inline String getName()
  {return this->name;}

  //setName
  inline void setName(String name)
  {this->name=name;}

  //getPolicy
  inline int getPolicy()
  {return this->policy;}

  //setPolicy
  inline void setPolicy(int value)
  {this->policy=value;}

  //isConnected
  inline bool isConnected()
  {return (this->inputs.size()+this->outputs.size())>0;}

  //isOutputConnectedTo
  inline bool isInputConnectedTo(DataflowPort* other)
  {return this->inputs.find(other)!=this->inputs.end();}

  //isOutputConnectedTo
  inline bool isOutputConnectedTo(DataflowPort* other)
  {return this->outputs.find(other)!=this->outputs.end();}

  //hasNewValue
  inline bool hasNewValue()
  {return !values.empty() && read_id<write_id;}

  //writeValue
  void writeValue(SharedPtr<DataflowValue> value,const SharedPtr<ReturnReceipt>& return_receipt=SharedPtr<ReturnReceipt>());

  //writeValue
  template <class Value>
  void writeValue(Value value, const SharedPtr<ReturnReceipt>& return_receipt = SharedPtr<ReturnReceipt>()) {
    writeValue(DataflowValue::wrapValue<Value>(value), return_receipt);
  }

  //readWriteTimestamp
  Int64 readWriteTimestamp();

  //readValue 
  SharedPtr<DataflowValue> readValue();

  //readValue 
  template <class Value>
  SharedPtr<Value> readValue() {
    return DataflowValue::unwrapValue<Value>(readValue());
  }

  //previewValue 
  DataflowPortValue* previewValue();

  //disconnect (both inputs and outputs)
  bool disconnect();

  //findFirstConnectedOutputOfType
  template <class ClassName>
  ClassName findFirstConnectedOutputOfType() const
  {
    for (auto it=outputs.begin();it!=outputs.end();it++)
      {if (ClassName ret=dynamic_cast<ClassName>((*it)->getNode())) return ret;}
    return nullptr;
  }

  //findFirstConnectedOutputOfType
  template <class ClassName>
  ClassName findFirstConnectedInputOfType() const
  {
    for (auto it=inputs.begin();it!=inputs.end();it++)
      {if (ClassName ret=dynamic_cast<ClassName>((*it)->getNode())) return ret;}
    return nullptr;
  }

protected:

  int       policy;
  Node*     node;  
  String    name;

  std::deque<DataflowPortValue> values;
  int write_id;
  int read_id;
};

} //namespace Visus

#endif //VISUS_DATAFLOW_PORT_H__

