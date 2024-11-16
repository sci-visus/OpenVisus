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

#ifndef VISUS_VOXELSCOOP_NODE_H_
#define VISUS_VOXELSCOOP_NODE_H_

#include <Visus/Gui.h>
#include <Visus/DataflowNode.h>
#include <Visus/Sphere.h>
#include <Visus/Graph.h>
#include <Visus/Array.h>
#include <Visus/GuiFactory.h>

#include <QFrame>

namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_GUI_API VoxelScoopNode : public Node
{
public:

  VISUS_NON_COPYABLE_CLASS(VoxelScoopNode)

  //_____________________________________________________
  class VISUS_GUI_API GraphNode
  {
  public:
    Sphere             s;
    bool               calculated_max_out_lengths=false;
    bool               calculated_max_in_length=false;
    std::vector<float> max_out_lengths;
    float              max_in_length=0;
    GraphNode(const Sphere &s_) : s(s_) {}
  };

  //______________________________________________________
  class VISUS_GUI_API GraphEdge
  {
  public:
    float length;
    std::vector<Sphere> pts;  // pts in this edge, not including end points
    GraphEdge(float len, const std::vector<Sphere> &pts_):length(len),pts(pts_){}
  };

  typedef Visus::Graph< GraphNode,GraphEdge > TrimGraph;

  //constructor
  VoxelScoopNode();

  //destructor
  ~VoxelScoopNode() {
  }

  //getTypeName
  virtual String getTypeName() const override {
    return "VoxelScoopNode";
  }

  //doSimplify
  bool doSimplify() const {
    return simplify;
  }

  //setSimplify
  void setSimplify(bool value) {
    setProperty("SetSimplify", this->simplify, value);
  }

  //getMinLength
  double getMinLength() const {
    return min_length;
  }

  //setMinLength
  void setMinLength(double value) {
    setProperty("SetMinLength", min_length, value);
  }

  //getMinRatio
  double getMinRatio() const {
    return min_ratio;
  }

  //setMinRatio
  void setMinRatio(double value) {
    setProperty("SetMinRatio", this->min_ratio, value);
  }

  //getThreshold
  double getThreshold() const {
    return threshold;
  }

  //setMinRatio
  void setThreshold(double value) {
    setProperty("SetThreshold", this->threshold, value);
  }

  //useMinimaAsSeed
  bool useMinimaAsSeed() const {
    return use_minima_as_seed;
  }

  //setUseMinimaAsSeed
  void setUseMinimaAsSeed(bool value) {
    setProperty("SetUseMinimaAsSeed",this->use_minima_as_seed,value);
  }

  //useMaximaAsSeed
  bool useMaximaAsSeed() const {
    return use_maxima_as_seed;
  }

  //setUseMaximaAsSeed
  void setUseMaximaAsSeed(bool value) {
    setProperty("SetUseMaximaAsSeed", this->use_maxima_as_seed, value);
  }

  //getMinDiam
  double getMinDiam() const {
    return min_diam;
  }

  //setMinDiam
  void setMinDiam(double value) {
    setProperty("SetMinDiam", this->min_diam, value);
  }

  //processInput
  virtual bool processInput() override;

  static VoxelScoopNode* castFrom(Node* obj) {
    return dynamic_cast<VoxelScoopNode*>(obj);
  }

  //createEditor
  virtual void createEditor();

public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

  friend class BuildVoxelScoop;

  class MyJob;
  friend class MyJob;

  bool    simplify = true;
  double  min_length = 10.0;
  double  min_ratio = 1.00;
  double  threshold = 42;
  bool    use_minima_as_seed = false;
  bool    use_maxima_as_seed = false;
  double  min_diam = 0.75;


  //the data on which I need to do voxelscoop calculation
  Array data;

  // seeds can come from merge tree or user selection
  std::vector<Point3i> seeds;      

}; //end class


} //namespace Visus

#endif // VISUS_VOXELSCOOP_NODE_H_
