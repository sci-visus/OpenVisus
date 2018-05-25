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

#include <Visus/StatisticsNode.h>

namespace Visus {


////////////////////////////////////////////////////////
class ComputeStatisticsJob : public NodeJob
{
public:

  StatisticsNode*  node;
  Array            data;

  //constructor
  ComputeStatisticsJob(StatisticsNode* node_,Array data_) : node(node_), data(data_){
  }

  //runJob
  virtual void runJob() override
  {
    Time t1=Time::now();
    if (auto stats=Statistics::compute(data,aborted))
    {
      VisusInfo()<<"Computed statistics done in "<<t1.elapsedMsec();
      node->publish(std::map<String, SharedPtr<Object> >({{"statistics",std::make_shared<Statistics>(stats)}}));
    }
  }
};

////////////////////////////////////////////////////////
StatisticsNode::StatisticsNode(String name) : Node(name) {
  addInputPort("data");
}

////////////////////////////////////////////////////////
StatisticsNode::~StatisticsNode() {
}

////////////////////////////////////////////////////////
bool StatisticsNode::processInput() 
{
  abortProcessing();
  auto data = readInput<Array>("data");
  if (!data) return false;
  VisusInfo()<<"Statistics node got data "<<data->dims.toString();
  addNodeJob(std::make_shared<ComputeStatisticsJob>(this,*data));
  return true;
}

////////////////////////////////////////////////////////
void StatisticsNode::messageHasBeenPublished(const DataflowMessage& msg) 
{
  VisusAssert(VisusHasMessageLock());

  auto statistics=std::dynamic_pointer_cast<Statistics>(msg.readContent("statistics"));
  if (!statistics)
    return;

  for (auto it: this->views)
  {
    if (auto view=dynamic_cast<BaseStatisticsNodeView*>(it)) {
      view->newStatsAvailable(*statistics);
    }
  }
}

} //namespace Visus
