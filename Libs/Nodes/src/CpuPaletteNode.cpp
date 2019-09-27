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

#include <Visus/CpuPaletteNode.h>

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////
class CpuPaletteNode::MyJob : public NodeJob
{
public:

  CpuPaletteNode*            node;
  Array                      input;
  SharedPtr<ReturnReceipt>   return_receipt;
  bool                       bDataOutputPortConnected;

  //constructor
  MyJob(CpuPaletteNode* node_,Array input_,SharedPtr<ReturnReceipt> return_receipt_) 
    : node(node_), input(input_),return_receipt(return_receipt_)
  {
    this->bDataOutputPortConnected=node->isOutputConnected("data");
  }

  //valid
  inline bool valid() {
    return node && input && bDataOutputPortConnected;
  }

  //runJob
  virtual void runJob() override
  {
    if (!valid() || aborted() || !input)
      return;

    Array output=node->transfer_function->applyToArray(input,this->aborted);

    if (!output)
      return;

    DataflowMessage msg;
    msg.setReturnReceipt(return_receipt);
    msg.writeValue("data",output);
    node->publish(msg);
  }
};


///////////////////////////////////////////////////////////////////////
CpuPaletteNode::CpuPaletteNode(String name,String default_palette)  : Node(name)
{
  addInputPort("data");
  addOutputPort("data");

  auto tf=std::make_shared<TransferFunction>();
  tf->setDefault(default_palette);
  setTransferFunction(tf);
}

///////////////////////////////////////////////////////////////////////
CpuPaletteNode::~CpuPaletteNode()
{
  setTransferFunction(nullptr);
}

///////////////////////////////////////////////////////////////////////
void CpuPaletteNode::setTransferFunction(SharedPtr<TransferFunction> value)
{
  if (this->transfer_function)
  {
    this->transfer_function->begin_update.disconnect(this->transfer_function_begin_update_slot);
    this->transfer_function->changed.disconnect(this->transfer_function_changed_slot);
  }

  this->transfer_function=value;

  if (this->transfer_function)
  {
    this->transfer_function->begin_update.connect(this->transfer_function_begin_update_slot=[this](){
      this->beginUpdate();
    });

    this->transfer_function->changed.connect(this->transfer_function_changed_slot=[this](){
      this->endUpdate();
    });
  }
}

///////////////////////////////////////////////////////////////////////
bool CpuPaletteNode::processInput()
{
  abortProcessing();

  // important to do before readValue
  auto return_receipt=createPassThroughtReceipt();

  auto data = readValue<Array>("data");
  if (!data)
    return false;

  auto job=std::make_shared<MyJob>(this,*data,return_receipt);
  if (!job->valid()) 
    return false;
  
  this->bounds=data->bounds;

  addNodeJob(job);
  return true;
}


///////////////////////////////////////////////////////////////////////
void CpuPaletteNode::writeToObjectStream(ObjectStream& out)
{
  Node::writeToObjectStream(out);
  out.writeObject("transfer_function",*transfer_function);
}

///////////////////////////////////////////////////////////////////////
void CpuPaletteNode::readFromObjectStream(ObjectStream& in)
{
  Node::readFromObjectStream(in);
  in.readObject("transfer_function",*transfer_function);
}


} //namespace Visus




