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

#include <Visus/PaletteNode.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////
class ComputeStatsJob : public NodeJob
{
public:

  Node*              node;
  Array              data;
  SharedPtr<Palette> palette;

  //constructor
  ComputeStatsJob(Node* node_,Array data_, SharedPtr<Palette> palette_)
    : node(node_), data(data_), palette(palette_){
  }

  //runJob
  virtual void runJob() override
  {
    if (auto stats = Statistics::compute(data, 256, aborted))
    {
      DataflowMessage msg;
      msg.writeValue("statistics", stats);
      node->publish(msg);
    }
  }
};



///////////////////////////////////////////////////////////////////////
PaletteNode::PaletteNode(String default_palette) 
{
  //in case you want to show statistics when you are editing the palette
  //to enable statistics, connect it
  addInputPort("array"); 
  addOutputPort("palette");

  setPalette(Palette::getDefault(default_palette));
}

///////////////////////////////////////////////////////////////////////
PaletteNode::~PaletteNode() {
  setPalette(nullptr);
}


///////////////////////////////////////////////////////////////////////
void PaletteNode::execute(Archive& ar)
{
  if (GetPassThroughAction("palette", ar))
  {
    palette->execute(ar);
    return;
  }

  return Node::execute(ar);
}

///////////////////////////////////////////////////////////////////////
void PaletteNode::setPalette(SharedPtr<Palette> value) 
{
  if (this->palette) 
  {
    this->palette->begin_update.disconnect(this->palette_begin_update_slot);
    this->palette->end_update.disconnect(this->palette_end_update_slot);
  }

  this->palette=value;
  doPublish();

  if (this->palette)
  {
    // a change in the palette means a change in the node 
    this->palette->begin_update.connect(this->palette_begin_update_slot=[this](){
      beginTransaction();
    });

    this->palette->end_update.connect(this->palette_end_update_slot = [this]() {
      addUpdate(
        CreatePassThroughAction("palette", palette->lastRedo()),
        CreatePassThroughAction("palette", palette->lastUndo()));
      endTransaction();
    });
  }
}


///////////////////////////////////////////////////////////////////////
bool PaletteNode::processInput()
{
  abortProcessing();

  //important to remove any input from the queue  
  auto data = readValue<Array>("array");
  if (!data) 
    return false;

  bool enable_statistics = getStatisticsEnabled() || (isInputConnected("array") && !views.empty());

  //not interested in statistics
  if (!enable_statistics)
    return false;

  addNodeJob(std::make_shared<ComputeStatsJob>(this,*data,palette));
  return true;
}

///////////////////////////////////////////////////////////////////////
void PaletteNode::enterInDataflow() {
  Node::enterInDataflow();
  doPublish();
}


///////////////////////////////////////////////////////////////////////
void PaletteNode::exitFromDataflow() {
  Node::exitFromDataflow();
}

///////////////////////////////////////////////////////////////////////
void PaletteNode::messageHasBeenPublished(DataflowMessage msg)
{
  VisusAssert(VisusHasMessageLock());

  auto statistics=msg.readValue<Statistics>("statistics");
  if (!statistics)
    return;

  this->last_statistics = *statistics;

  for (auto it: this->views)
  {
    if (auto view=dynamic_cast<BasePaletteNodeView*>(it)) {
      view->newStatsAvailable(*statistics);
    }
  }
}

///////////////////////////////////////////////////////////////////////
void PaletteNode::write(Archive& ar) const
{
  Node::write(ar);
  ar.write("statistics_enabled", this->statistics_enabled);
  ar.writeObject("palette", *palette);
}


///////////////////////////////////////////////////////////////////////
void PaletteNode::read(Archive& ar)
{
  Node::read(ar);
  this->statistics_enabled = ar.readBool("statistics_enabled", false);
  ar.readObject("palette", *palette);
}


} //namespace Visus



