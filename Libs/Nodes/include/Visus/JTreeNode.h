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

#ifndef VISUS_JTREE_NODE_H_
#define VISUS_JTREE_NODE_H_

#include <Visus/Nodes.h>
#include <Visus/DataflowNode.h>
#include <Visus/Array.h>
#include <Visus/Graph.h>

namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_NODES_API JTreeNode : public Node
{
public:

  VISUS_NON_COPYABLE_CLASS(JTreeNode)

  // construct 
  JTreeNode(String name=""); 

  //destructor
  ~JTreeNode();

  //getMinimaTree
  bool getMinimaTree() const {
    return minima_tree;
  }
  
  //setMinimaTree
  void setMinimaTree(bool value) {
    if (minima_tree==value) return;
    beginUpdate();
    minima_tree=value;
    endUpdate();
    recompute();
  }

  //getMinPersistence
  double getMinPersistence() const {
    return min_persistence;
  }

  //setMinPersistence
  void setMinPersistence(double value) {
    if (min_persistence==value) return;
    beginUpdate();
    min_persistence=value;
    endUpdate();
    recompute(false);
  }

  //getReduceMinMax
  bool getReduceMinMax() const {
    return reduce_minmax;
  }

  //setReduceMinMax
  void setReduceMinMax(bool value) {
    if (reduce_minmax==value) return;
    beginUpdate();
    reduce_minmax=value;
    endUpdate();
    recompute(false);
  }

  //getThresholdMin
  double getThresholdMin() const {
    return threshold_min;
  }

  //setThresholdMin
  void setThresholdMin(double value) {
    if (threshold_min==value) return;
    beginUpdate();
    threshold_min=value;
    endUpdate();
    recompute();
  }


  //getThresholdMax
  double getThresholdMax() const {
    return threshold_max;
  }

  //setThresholdMax
  void setThresholdMax(double value) {
    if (threshold_max==value) return;
    beginUpdate();
    threshold_max=value;
    endUpdate();
    recompute();
  }

  //getAutoThreshold
  bool getAutoThreshold() const {
    return auto_threshold;
  }

  //setAutoThreshold
  void setAutoThreshold(bool value) {
    if (auto_threshold==value) return;
    beginUpdate();
    auto_threshold=value;
    endUpdate();
    recompute();
  }

  //processInput
  virtual bool processInput() override;

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  template <typename Type> 
  class MyJob;

  template <typename Type> 
  friend class MyJob;

  //properties
  bool    minima_tree=false; // if true construct a minima tree
  double  min_persistence=21; // minimum persistence to retain nodes
  bool    reduce_minmax=false; // whether or not to reduce min-max edges if they are shorter than min_persist
  double  threshold_min=0;// (just dynamic_pointer_cast this to dtype)
  double  threshold_max=0;
  bool    auto_threshold=true; // set threshold automatically

  //the data on which I need to do jtree calculation
  Array  data;
  
  double minThresholdRange=-1;
  double maxThresholdRange=+1;

  SharedPtr<BaseGraph> last_full_graph;

  //update automatic threshold
  void updateAutoThreshold();

  //messageHasBeenPublished
  virtual void messageHasBeenPublished(DataflowMessage msg) override;

  //recompute
  bool recompute(bool bFull=true);

};


} //namespace Visus

#endif // VISUS_JTREE_NODE_H_
