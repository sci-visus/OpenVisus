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

  Node* node;
  Array              data;
  SharedPtr<Palette> palette;

  //constructor
  ComputeStatsJob(Node* node_,Array data_, SharedPtr<Palette> palette_)
    : node(node_), data(data_), palette(palette_){
  }

  //runJob
  virtual void runJob() override
  {
    std::vector<Range> ranges;
    for (int C = 0; C < data.dtype.ncomponents(); C++)
      ranges.push_back(palette->computeRange(data, C, aborted));

    if (auto stats = Statistics::compute(data, ranges, 256, aborted))
    {
      DataflowMessage msg;
      msg.writeValue("statistics", stats);
      node->publish(msg);
    }
  }
};



///////////////////////////////////////////////////////////////////////
PaletteNode::PaletteNode(String name,String default_palette) : Node(name)
{
  //in case you want to show statistics when you are editing the palette
  //to enable statistics, connect it
  addInputPort("data"); 
  addOutputPort("palette");

  setPalette(Palette::getDefault(default_palette));
}

///////////////////////////////////////////////////////////////////////
PaletteNode::~PaletteNode() {
  setPalette(nullptr);
}


///////////////////////////////////////////////////////////////////////
void PaletteNode::executeAction(StringTree in)
{
  if (getPassThroughAction(in, "palette"))
  {
    palette->executeAction(in);
    return;
  }

  return Node::executeAction(in);
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

  if (this->palette)
  {
    // a change in the palette means a change in the node 
    this->palette->begin_update.connect(this->palette_begin_update_slot=[this](){
      beginUpdate(
        StringTree("__change__"),
        StringTree("__change__"));
    });

    this->palette->end_update.connect(this->palette_end_update_slot = [this]() {
      topRedo() = createPassThroughAction(palette->topRedo(), "palette");
      topUndo() = createPassThroughAction(palette->topUndo(), "palette");
      endUpdate();
    });
  }
}


///////////////////////////////////////////////////////////////////////
bool PaletteNode::processInput()
{
  abortProcessing();

  //important to remove any input from the queue  
  auto data = readValue<Array>("data");
  if (!data) 
    return false;

  //not interested in statistics
  if (!areStatisticsEnabled())
    return false;

  //avoid computing stuff if not shown
  if (views.empty())
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

  for (auto it: this->views)
  {
    if (auto view=dynamic_cast<BasePaletteNodeView*>(it)) {
      view->newStatsAvailable(*statistics);
    }
  }
}

///////////////////////////////////////////////////////////////////////
void PaletteNode::writeTo(StringTree& out) const
{
  Node::writeTo(out);
  out.writeObject("palette", *palette);
}


///////////////////////////////////////////////////////////////////////
void PaletteNode::readFrom(StringTree& in)
{
  Node::readFrom(in);
  in.readObject("palette", *palette);
}


} //namespace Visus



