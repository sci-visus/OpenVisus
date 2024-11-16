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

#ifndef __VISUS_QUERYNODE_H
#define __VISUS_QUERYNODE_H

#include <Visus/Nodes.h>
#include <Visus/DatasetNode.h>
#include <Visus/Dataflow.h>

namespace Visus {

  ///////////////////////////////////////////////////////////////////////////////////////////////
class VISUS_NODES_API QueryNode : public Node
{
public:

  VISUS_NON_COPYABLE_CLASS(QueryNode)

#if !SWIG
  static std::function<bool(Int64)> willFitOnGpu;
#endif

  //constructor
  QueryNode();

  //destructor
  virtual ~QueryNode();

  //getTypeName
  virtual String getTypeName() const override {
    return "QueryNode";
  }

  //getDataset()
  SharedPtr<Dataset> getDataset();

  //the dataflow dataset
  DatasetNode* getDatasetNode();

  //getField
  Field getField();

  //getTime
  double getTime();

  //processInput
  virtual bool processInput() override;

  //isVerbose
  bool isVerbose() const {
    return this->verbose;
  }

  //setVerbose
  void setVerbose(int value) {
    this->setProperty("SetVerbose", this->verbose, value);
  }

  //getAccessIndex
  int getAccessIndex() const {
    return this->accessindex;
  }

  //setAccessIndex
  void setAccessIndex(int value) {
    this->setProperty("SetAccessIndex", this->accessindex, value);
    this->access.reset();
  }

  //setAccess
  void setAccess(SharedPtr<Access> value) {
    this->access=value;
  }

  //getProgression
  int getProgression() const {
    return progression;
  }

  //setProgression
  void setProgression(int value) {
    setProperty("SetProgression", this->progression, value);
  }

  //getQuality
  int getQuality() const {
    return quality;
  }

  //setQuality
  void setQuality(int value) {
    setProperty("SetQuality", this->quality, value);
  }

  //getAccuracy
  double getAccuracy() const {
    return accuracy;
  }

  //setQuality
  void setAccuracy(double value) {
    setProperty("SetAccuracy", this->accuracy, value);
  }

  //getBounds
  virtual Position getBounds() override {
    return node_bounds;
  }

  //setBounds
  void setBounds(Position value);

  //getQueryBounds
  Position getQueryBounds() const {
    return query_bounds;
  }

  //setQueryBounds
  void setQueryBounds(Position value) {
    this->query_bounds =value;
  }

  //getQueryLogicPosition
  Position getQueryLogicPosition();

  //nodeToScreen
  Frustum nodeToScreen() const {
    return view_dependent_enabled ? node_to_screen : Frustum();
  }

  //logicToScreen
  Frustum logicToScreen();

  //setNodeToScreen
  void setNodeToScreen(Frustum value) {
    this->node_to_screen =value;
  }

  //isViewDependentEnabled
  bool isViewDependentEnabled() const {
    return view_dependent_enabled;
  }

  //setViewDependentEnabled
  void setViewDependentEnabled(bool value) {
    if (view_dependent_enabled ==value) return;
    setProperty("SetViewDependentEnabled", this->view_dependent_enabled, value);
  }

  //exitFromDataflow (to avoid dataset stuck in memory)
  virtual void exitFromDataflow() override;

  //castFrom
  static QueryNode* castFrom(Node* obj) {
    return dynamic_cast<QueryNode*>(obj);
  }
public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

protected:

  SharedPtr<Access> access;

private:

  class MyJob;
  friend class MyJob;

  //properties
  int                verbose = 0;
  int                accessindex=0;
  bool               view_dependent_enabled = false;
  int                progression = QueryGuessProgression;
  int                quality = QueryDefaultQuality;
  Position           node_bounds = Position::invalid();
  double             accuracy = 0.0; //for idx2

  //run time derived properties
  Frustum            node_to_screen;
  Position           query_bounds;

  //modelChanged
  virtual void modelChanged() override {
    if (dataflow)
      dataflow->needProcessInput(this);
  }

  //publishDumbArray
  void publishDumbArray();

  
}; 

} //namespace Visus

#endif //__VISUS_QUERYNODE_H

