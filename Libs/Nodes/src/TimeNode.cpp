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

#include <Visus/TimeNode.h>

namespace Visus {


//////////////////////////////////////////////////
TimeNode::TimeNode(String name,double current_time_,DatasetTimesteps timesteps_) 
  : Node(name),current_time(current_time_),timesteps(timesteps_),play_msec(1000)
{
  this->user_range=timesteps.getRange();
  addOutputPort("time");
}

//////////////////////////////////////////////////
TimeNode::~TimeNode()
{
}


//////////////////////////////////////////////////////////////////////////
void TimeNode::executeAction(StringTree action)
{
  if (action.name == "SetProperty")
  {
    auto name = action.readString("name");

    if (name == "current_time") {
      setCurrentTime(action.readDouble("value"));
      return;
    }

    if (name == "user_range") {
      setUserRange(Range::fromString(action.readString("value")));
      return;
    }

    if (name == "play_msec") {
      setPlayMsec(action.readInt("value"));
      return;
    }

  }

  return Node::executeAction(action);
}

//////////////////////////////////////////////////
void TimeNode::setCurrentTime(double value,bool bDoPublish)
{
  //NOTE: I accept even value if not in timesteps...
  if (this->current_time!=value)
  {
    setProperty("current_time", this->current_time, value);

    if (bDoPublish)
      doPublish();
  }
}

//////////////////////////////////////////////////
void TimeNode::setUserRange(const Range& value)
{
  if (this->user_range==value)
    return;

  setProperty("user_range", this->user_range, value);
  doPublish();
}

//////////////////////////////////////////////////
void TimeNode::setPlayMsec(int value)
{
  if (this->play_msec==value) return;
  setProperty("play_msec", this->play_msec, value);
  //doPublish();
}

//////////////////////////////////////////////////
void TimeNode::enterInDataflow() 
{
  Node::enterInDataflow();
  doPublish();
}

//////////////////////////////////////////////////
void TimeNode::exitFromDataflow() 
{
  Node::exitFromDataflow();
}

//////////////////////////////////////////////////
void TimeNode::doPublish(SharedPtr<ReturnReceipt> return_receipt)
{
  if (!getDataflow()) 
    return;
  
  DataflowMessage msg;
  msg.setReturnReceipt(return_receipt);
  msg.writeValue("time",current_time);
  this->publish(msg);
}

//////////////////////////////////////////////////
void TimeNode::writeTo(StringTree& out) const
{
  Node::writeTo(out);

  out.writeValue("current_time",cstring(current_time));

  out.writeObject("timesteps", timesteps);

  if (user_range!=timesteps.getRange())
    out.writeObject("user_range", user_range);

  out.writeValue("play_msec",cstring(play_msec));
}

//////////////////////////////////////////////////
void TimeNode::readFrom(StringTree& in) 
{
  Node::readFrom(in);

  current_time=cdouble(in.readValue("current_time"));

  in.readObject("timesteps", timesteps);

  user_range=timesteps.getRange();
  in.readObject("user_range", user_range);

  play_msec=cint(in.readValue("play_msec","1000"));
}


} //namespace Visus









