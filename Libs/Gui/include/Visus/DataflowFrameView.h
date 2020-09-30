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

#ifndef _VISUS_DATAFLOW_FRAME_VIEW_H__
#define _VISUS_DATAFLOW_FRAME_VIEW_H__

#include <Visus/Gui.h>
#include <Visus/Model.h>
#include <Visus/Dataflow.h>
#include <Visus/Color.h>
#include <Visus/Gui.h>

#include <QFrame>
#include <QTimer>
#include <QMouseEvent>

namespace Visus {

//////////////////////////////////////////////////////////////
class VISUS_GUI_API DataflowFrameView : 
  public QFrame , 
  public Dataflow::Listener
{
public:

  VISUS_NON_COPYABLE_CLASS(DataflowFrameView)

  //constructor
  DataflowFrameView(Dataflow* dataflow);

  //destructor
  virtual ~DataflowFrameView();

  //setDataflow
  void setDataflow(Dataflow* dataflow) ;

private:

  class NodeWidget;
  friend class NodeWidget;

  Dataflow* dataflow=nullptr;

  std::map<Node*, QWidget*> get_widget;
  std::map<QWidget*, Node*> get_node;

  struct
  {
    bool          enabled;
    Node*         dragging;
    Time          started;
    QTimer        timer;
  }
  elastic_layout;

  //getNode
  inline Node* getNode(QWidget* widget){
    auto it = get_node.find(widget); return it == get_node.end() ? nullptr : it->second;
  }

  //getWidget
  inline QWidget* getWidget(Node* node) {
    auto it = get_widget.find(node); return it == get_widget.end() ? nullptr : it->second;
  }

  virtual void paintEvent(QPaintEvent* evt) override;

  //addNode
  void addNode(Node*);

  //computeElasticLayout
  void computeElasticLayout();

  //getInputPortPosition
  Point2d getInputPortPosition(DataflowPort* iport);

  //getOutputPortPosition
  Point2d getOutputPortPosition(DataflowPort* oport);

  //setElasticDraggingNode
  void setElasticDraggingNode(Node* node);

  //normalizedGeometry
  Rectangle2d normalizedGeometry(QWidget* widget);

  //setNormalizedGeometry
  void setNormalizedGeometry(QWidget* widget, Rectangle2d r);

  //refreshBounds
  void refreshBounds();

  //resizeEvent
  virtual void resizeEvent(QResizeEvent* evt) override;

  //showEvent
  virtual void showEvent(QShowEvent *evt) override;

  //hideEvent
  virtual void hideEvent(QHideEvent *evt) override;

  //dataflowBeingDestroyed
  virtual void dataflowBeingDestroyed() override {
    setDataflow(nullptr);
  }

  //dataflowSetNodeName
  virtual void dataflowSetNodeName(Node* node,String old_value,String new_value) override;

  //dataflowSetNodeVisible
  virtual void dataflowSetNodeVisible(Node* node,bool old_value,bool new_value) override;

  //dataflowAddNode
  virtual void dataflowAddNode(Node* node) override;

  //dataflowRemoveNode
  virtual void dataflowRemoveNode(Node* node) override;

  //dataflowMoveNode
  virtual void dataflowMoveNode(Node* dst,Node* src,int index) override;

  //dataflowSetSelection
  virtual void dataflowSetSelection(Node* old_selection,Node* new_selection) override;

  //datafloConnectNodes
  virtual void dataflowConnectNodes(Node* from,String oport,String iport,Node* to) override;

  //datafloConnectNodes
  virtual void dataflowDisconnectNodes(Node* from,String oport,String iport,Node* to) override;

};

} //namespace Visus

#endif //_VISUS_DATAFLOW_FRAME_VIEW_H__




