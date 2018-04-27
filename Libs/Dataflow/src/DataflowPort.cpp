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
#include <Visus/DataflowPort.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////////
void DataflowPort::writeValue(SharedPtr<Object> value,const SharedPtr<ReturnReceipt>& return_receipt)
{
  if (policy==DoNotStoreValue) 
    return;

  if (policy!=StoreMultipleVolatileValues) 
    values.clear();

  DataflowPortStoredValue dataflow_port_stored_value;
  dataflow_port_stored_value.value=value;
  dataflow_port_stored_value.write_id=++write_id;
  dataflow_port_stored_value.write_timestamp=Time::getTimeStamp();
  if (return_receipt)
  {
    dataflow_port_stored_value.return_receipt=return_receipt;
    return_receipt->needSignature(this);
  }
  this->values.push_back(dataflow_port_stored_value);
}

//////////////////////////////////////////////////////////////////////////
SharedPtr<Object> DataflowPort::readValue(Int64* write_timestamp)
{
  if (this->values.empty()) return SharedPtr<Object>();
  VisusAssert(policy!=DoNotStoreValue);
  SharedPtr<Object> ret=values.front().value;
  if (write_timestamp) (*write_timestamp)=values.front().write_timestamp;
  this->read_id=values.front().write_id;
  if (SharedPtr<ReturnReceipt> return_receipt=values.front().return_receipt)
  {
    return_receipt->addSignature(this);
    values.front().return_receipt.reset(); //sign just one time
  }
  if (policy!=StoreOnlyOnePersistentValue) values.pop_front();
  return ret;
}

//////////////////////////////////////////////////////////////////////////
DataflowPortStoredValue* DataflowPort::previewValue()
{
  if (this->values.empty()) return nullptr;
  VisusAssert(policy!=DoNotStoreValue);
  return &values.front();
}

//////////////////////////////////////////////////////////////////////////
bool DataflowPort::disconnect()
{
  VisusAssert(VisusHasMessageLock());

  //disconnect all inputs
  for (iterator it=this->inputs.begin();it!=this->inputs.end();it++)
  {
    DataflowPort* port=*it;
    VisusAssert(port->outputs.find(this)!=port->outputs.end());
    port->outputs.erase(this);
  }
  this->inputs.clear();

  //disconnect all outputs
  for (iterator it=this->outputs.begin();it!=this->outputs.end();it++) 
  {
    DataflowPort* port=*it;
    VisusAssert(port->inputs.find(this)!=port->inputs.end());
    port->inputs.erase(this);
  }
  this->outputs.clear();

  //also reset the internal value
  this->values.clear();
  this->read_id=0;
  this->write_id=0;
  return true;
}

} //namespace Visus 

