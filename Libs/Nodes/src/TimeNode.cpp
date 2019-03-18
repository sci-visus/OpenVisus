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

//////////////////////////////////////////////////
void TimeNode::setCurrentTime(double value,bool bDoPublish)
{
  //NOTE: I accept even value if not in timesteps...
  if (this->current_time!=value)
  {
    beginUpdate();
    this->current_time=value;
    endUpdate();

    if (bDoPublish)
      doPublish();
  }
}

//////////////////////////////////////////////////
void TimeNode::setUserRange(const Range& value)
{
  if (this->user_range==value)
    return;

  beginUpdate();
  this->user_range=value;
  endUpdate();

  doPublish();
}

//////////////////////////////////////////////////
void TimeNode::setPlayMsec(int value)
{
  if (this->play_msec==value)
    return;

  beginUpdate();
  this->play_msec=value;
  endUpdate();

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
  
  auto msg=std::make_shared<DataflowMessage>();
  msg->setReturnReceipt(return_receipt);
  msg->writeContent("time",std::make_shared<DoubleObject>(current_time));
  this->publish(msg);
}

//////////////////////////////////////////////////
void TimeNode::writeToObjectStream(ObjectStream& ostream) 
{
  if (ostream.isSceneMode())
  {
    ostream.pushContext("timestep");

    ostream.pushContext("keyframes");
    ostream.writeInline("interpolation", "none");

    // TODO loop through the keyframes for this object
    ostream.pushContext("keyframe");
    ostream.writeInline("time", "0");

    ostream.pushContext("time");
    ostream.writeInline("value", cstring(current_time));
    ostream.popContext("time");

    ostream.popContext("keyframe");
    ostream.popContext("keyframes");
    // TODO end loop

    ostream.popContext("timestep");
    return;
  }

  Node::writeToObjectStream(ostream);

  ostream.write("current_time",cstring(current_time));

  ostream.pushContext("timesteps");
  timesteps.writeToObjectStream(ostream);
  ostream.popContext("timesteps");

  if (user_range!=timesteps.getRange())
  {
    ostream.pushContext("user_range");
    user_range.writeToObjectStream(ostream);
    ostream.popContext("user_range");
  }

  ostream.write("play_msec",cstring(play_msec));
}

//////////////////////////////////////////////////
void TimeNode::readFromObjectStream(ObjectStream& istream) 
{
  Node::readFromObjectStream(istream);

  current_time=cdouble(istream.read("current_time"));

  if (istream.pushContext("timesteps"))
  {
    timesteps.readFromObjectStream(istream);
    istream.popContext("timesteps");
  }

  user_range=timesteps.getRange();
  if (istream.pushContext("user_range"))
  {
    user_range.readFromObjectStream(istream);
    istream.popContext("user_range");
  }

  play_msec=cint(istream.read("play_msec","1000"));
}


} //namespace Visus









