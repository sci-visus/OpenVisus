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

#include <Visus/FunnelNode.h>

namespace Visus {

//////////////////////////////////////////////////////////////
bool FunnelNode::processInput()
{
  SharedPtr<DataflowValue> latest_value;
  Int64 latest_write_timestamp=-1;

  //read in inputs
  for (auto it=this->inputs.begin();it!=this->inputs.end();it++)
  {
    DataflowPort* iport=it->second;

    if (!iport->hasNewValue()) 
      continue;

    Int64 write_timestamp=iport->readWriteTimestamp();
    SharedPtr<DataflowValue>  value = iport->readValue();

    if (value && (latest_write_timestamp==-1 || write_timestamp>latest_write_timestamp))
    {
      latest_write_timestamp = write_timestamp;
      latest_value           = value;
    }
  }

  if (latest_value)
  {
    DataflowMessage msg;
    for (auto it=this->outputs.begin();it!=this->outputs.end();it++)
    {
      String oport_name=it->first;
      msg.writeValue(oport_name,latest_value);
    }
    this->publish(msg);
  }
  return true;
}


////////////////////////////////////////////////////////////////////////
void FunnelNode::writeTo(StringTree& out) const
{
  Node::writeTo(out);
  VisusAssert(false); //todo: i need to read write input output ports
}

////////////////////////////////////////////////////////////////////////
void FunnelNode::readFrom(StringTree& in)
{
  Node::readFrom(in);
  VisusAssert(false); //todo: i need to read write input output ports
}

} //namespace Visus 
