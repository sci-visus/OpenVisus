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

#ifndef _VISUS_DATAFLOW_TREE_VIEW_H__
#define _VISUS_DATAFLOW_TREE_VIEW_H__

#include <Visus/Gui.h>
#include <Visus/Model.h>
#include <Visus/Dataflow.h>

#include <QTreeWidget>

namespace Visus {

////////////////////////////////////////////////////////////////
class VISUS_GUI_API DataflowTreeView : 
  public QTreeWidget , 
  public Dataflow::Listener
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(DataflowTreeView)

  //constructor
  DataflowTreeView(Dataflow* dataflow);

  //destructor
  virtual ~DataflowTreeView();

  //setDataflow
  void setDataflow(Dataflow* dataflow) ;

  //get_icon
  std::function<QIcon(Node*)> getIcon=std::function<QIcon(Node*)>([](Node*){return QIcon();});

  //getNode
  inline Node* getNode(QTreeWidgetItem* widget)
  {auto it=get_node.find(widget);return it==get_node.end()? nullptr : it->second;}

  //setExpanded
  inline void setExpanded(Node* snode,bool value)
  {if (QTreeWidgetItem* widget=getWidget(snode)) widget->setExpanded(value);}

signals:

  //moveNodeRequest
  void moveNodeRequest(Node* dst,Node* src,int index);

private:

  Dataflow* dataflow=nullptr;

  std::map<Node*,QTreeWidgetItem*> get_widget;
  std::map<QTreeWidgetItem*,Node*> get_node;

  struct
  {
    DropIndicatorPosition pos;
    QRect rect;
  }
  indicator;

  //getWidget
  inline QTreeWidgetItem* getWidget(Node* snode) {
    auto it = get_widget.find(snode); return it == get_widget.end() ? nullptr : it->second;
  }

  //createTreeWidgetItem
  QTreeWidgetItem* createTreeWidgetItem(Node* node);

  //startDrag
  virtual void dragEnterEvent(QDragEnterEvent *event) override;

  //dragMoveEvent
  virtual void dragMoveEvent(QDragMoveEvent* event) override;

  //dragLeaveEvent
  virtual void dragLeaveEvent(QDragLeaveEvent *event) override;

  //dropEvent
  virtual void dropEvent(QDropEvent *evt) override;

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

  //paintEvent
  virtual void paintEvent(QPaintEvent * event) override;


};

} //namespace Visus

#endif //_VISUS_DATAFLOW_TREE_VIEW_H__




