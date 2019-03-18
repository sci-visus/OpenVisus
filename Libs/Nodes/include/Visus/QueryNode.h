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

  //internal use only
  static bool bDisableFindQUeryIntersectionWithDatasetBox;

  //constructor
  QueryNode(String name="");

  //destructor
  virtual ~QueryNode();

  //processInput
  virtual bool processInput() override;

  //the dataflow dataset
  virtual DatasetNode* getDatasetNode();

  //getDataset()
  virtual SharedPtr<Dataset> getDataset();

  //getField
  virtual Field getField();

  //getAccessIndex
  int getAccessIndex() const {
    return this->accessindex;
  }

  //setAccessIndex
  void setAccessIndex(int value) {
    this->beginUpdate();
    this->accessindex=value;
    setAccess(SharedPtr<Access>());
    this->endUpdate();
  }

  //setAccess
  void setAccess(SharedPtr<Access> value) {
    this->access=value;
  }

  //getProgression
  Query::Progression getProgression() const {
    return progression;
  }

  //setProgression
  void setProgression(Query::Progression value) {
    if (value==getProgression()) return;
    beginUpdate();
    this->progression=value;
    endUpdate();
  }

  //getQuality
  Query::Quality getQuality() const {
    return quality;
  }

  //setQuality
  void setQuality(Query::Quality value) {
    if (value==getQuality()) return;
    beginUpdate();
    this->quality=value;
    endUpdate();
  }

  //getNodeBounds
  virtual Position getNodeBounds() override {
    return bounds;
  }

  //setNodeBounds
  void setNodeBounds(Position value,bool bForce=false) {
    if (bounds==value && !bForce) return;
    beginUpdate();
    this->bounds=value;
    endUpdate();
  }

  //getQueryPosition
  Position getQueryPosition() const {
    return position;
  }

  //setQueryPosition
  void setQueryPosition(Position value) {
    this->position=value;
  }

  //getViewDep
  Frustum getViewDep() const {
    return bViewDependentEnabled? viewdep : Frustum();
  }

  //setViewDep
  void setViewDep(Frustum value) {
    this->viewdep=value;
  }

  //isViewDependentEnabled
  bool isViewDependentEnabled() const {
    return bViewDependentEnabled;
  }

  //setViewDependentEnabled
  void setViewDependentEnabled(bool value) {
    if (bViewDependentEnabled ==value) return;
    beginUpdate();
    this->bViewDependentEnabled =value;
    endUpdate();
  }

  //exitFromDataflow (to avoid dataset stuck in memory)
  virtual void exitFromDataflow() override;

  static QueryNode* castFrom(Node* obj) {
    return dynamic_cast<QueryNode*>(obj);
  }
public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

protected:

  SharedPtr<Access> access;

private:

  class MyJob;
  friend class MyJob;

  int                accessindex=0;
  bool               bViewDependentEnabled = false;
  Frustum            viewdep;
  Query::Progression progression=Query::GuessProgression;
  Query::Quality     quality=Query::DefaultQuality;
  Position           bounds=Position::invalid();
  Position           position;

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

