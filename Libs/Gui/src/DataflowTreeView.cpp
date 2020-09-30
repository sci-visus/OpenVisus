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

#include <Visus/DataflowTreeView.h>

#include <QMoveEvent>
#include <QProxyStyle>
#include <QPainter>

namespace Visus {

  /////////////////////////////////////////////////////////////////////////
static void SetWidgetVisible(QTreeWidgetItem* widget,bool value)
{
  //if I set to disabled, the node cannot be selected
  //widget->setDisabled(!value);

  widget->setForeground(0, QBrush(QColor(0, 0, 0, value ? 255 : 60)));
  for (int I = 0; I < widget->childCount(); I++)
    SetWidgetVisible(widget->child(I), value);
}


/////////////////////////////////////////////////////////////////////////
DataflowTreeView::DataflowTreeView(Dataflow* dataflow)
{
  setHeaderHidden(true);
  setIndentation(10);
  //setIconSize(QSize(24,24));
  setStyleSheet("QTreeView::item { padding: 4px 0px; }");

  //enable dragging (see https://stackoverflow.com/questions/21283934/qtreewidget-reordering-child-items-by-dragging)
  setSelectionMode(QAbstractItemView::SingleSelection);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDragDropMode(QAbstractItemView::InternalMove);

  setDataflow(dataflow);
}

/////////////////////////////////////////////////////////////////////////
DataflowTreeView::~DataflowTreeView()
{
  setDataflow(nullptr);
}


/////////////////////////////////////////////////////////////////////////
QTreeWidgetItem* DataflowTreeView::createTreeWidgetItem(Node* node)
{
  QTreeWidgetItem* widget=new QTreeWidgetItem();
  get_widget[node]=widget;
  get_node[widget]=node;
  widget->setText(0,node->getName().c_str());
  widget->setIcon(0,getIcon(node));
  SetWidgetVisible(widget, node->isVisible());
  return widget;
}

/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::setDataflow(Dataflow* value)
{
  if (this->dataflow)
  {
    QTreeWidget::clear();
    get_widget.clear();
    get_node.clear();
    Utils::remove(this->dataflow->listeners,this);
  }
  
  this->dataflow=value;
  
  if (this->dataflow)
  {
    this->dataflow->listeners.push_back(this);

    //rebuild the treeview from scratch
    if (auto root = dataflow->getRoot())
    {
      for (auto node : root->breadthFirstSearch())
      {
        QTreeWidgetItem* widget = createTreeWidgetItem(node);

        if (node == root)
        {
          QTreeWidget::clear();
          addTopLevelItem(widget);
        }
        else
          getWidget(node->getParent())->addChild(widget);
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowSetNodeName(Node* node,String old_value,String new_value) 
{
  if (QTreeWidgetItem* widget=getWidget(node))
    widget->setText(0,new_value.c_str());
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowSetNodeVisible(Node* node,bool old_value,bool new_value) 
{
  if (QTreeWidgetItem* widget = getWidget(node))
    SetWidgetVisible(widget, new_value);
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowAddNode(Node* node) 
{
  VisusAssert(node->getChilds().empty());

  QTreeWidgetItem* widget=createTreeWidgetItem(node);

  if (Node* parent=node->getParent())
  {
    QTreeWidgetItem* parent_widget=getWidget(parent);VisusAssert(parent_widget);
    parent_widget->setIcon(0,getIcon(parent)); //could be that the icon needs to be changed
    parent_widget->insertChild(node->getIndexInParent(),widget);
  }
  else
  {
    QTreeWidget::clear();
    addTopLevelItem(widget);
  }
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowRemoveNode(Node* node) 
{
  QTreeWidgetItem* widget=getWidget(node);
  VisusAssert(widget);
    
  if (Node* parent=node->getParent())
  {
    QTreeWidgetItem* parent_widget=getWidget(parent); VisusAssert(parent_widget);
    parent_widget->removeChild(widget);
  }
  else
  {
    QTreeWidget::clear();
  }
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowMoveNode(Node* dst,Node* src,int index) 
{
  QTreeWidgetItem* child_widget =getWidget(src); VisusAssert(child_widget);
  {
    QTreeWidgetItem* parent_widget=child_widget->parent();VisusAssert(parent_widget);
    parent_widget->setIcon(0,getIcon(getNode(parent_widget)));
    parent_widget->removeChild(child_widget);
  }
    
  {
    QTreeWidgetItem* parent_widget=getWidget(dst);
    parent_widget->setIcon(0,getIcon(dst));//could be that the icon needs to be changed
    if (index<0)
      parent_widget->addChild(child_widget);
    else
      parent_widget->insertChild(index,child_widget);
  }
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowSetSelection(Node* old_selection,Node* new_selection) 
{
  QTreeWidget::setCurrentItem(getWidget(new_selection));
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowConnectNodes(Node* from,String oport,String iport,Node* to)
{
  //don't care
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dataflowDisconnectNodes(Node* from,String oport,String iport,Node* to)
{
  //don't care
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dragEnterEvent(QDragEnterEvent* evt)
{
  QTreeWidget::dragEnterEvent(evt);
  indicator.rect = QRect();
  indicator.pos = OnViewport;
  repaint();
}


/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::dragMoveEvent(QDragMoveEvent* evt) 
{
  QTreeWidget::dragMoveEvent(evt);

  indicator.pos = OnViewport;
  indicator.rect = QRect();

  //see https://github.com/jimmykuu/PyQt-PySide-Cookbook/blob/master/tree/drop_indicator.md
  auto pos = evt->pos();
  if (auto dst = itemAt(pos))
  {
    auto index = indexFromItem(dst);
    auto rect = visualRect(index);

    const int margin = 10;
    if (pos.y() - rect.top() < margin)
    {
      indicator.pos = AboveItem;
      rect.setHeight(0);
    }
    else if (rect.bottom() - pos.y() < margin)
    {
      indicator.pos = BelowItem;
      rect = QRect(rect.left(), rect.bottom(), rect.right() - rect.left(), 0);
    }
    else if (pos.y() - rect.top() > margin && rect.bottom() - pos.y() > margin)
    {
      indicator.pos = OnItem;
    }
    else 
    {
      indicator.pos = OnViewport;
      rect = QRect();
    }
    
    indicator.rect = rect;
  }

  repaint();
}


/////////////////////////////////////////////////////////
void DataflowTreeView::dragLeaveEvent(QDragLeaveEvent *evt) 
{
  QTreeWidget::dragLeaveEvent(evt);
  indicator.pos = OnViewport;
  indicator.rect = QRect();
  repaint();
}

/////////////////////////////////////////////////////////
void DataflowTreeView::dropEvent(QDropEvent *evt) 
{
  auto src = getNode(this->currentItem()); 
  auto dst = getNode(itemAt(evt->pos())); 

  //do not want QT really take care of the action!
  evt->setDropAction(Qt::IgnoreAction);

  //default is at the end
  auto index = -1;

  switch (indicator.pos)
  {
  case OnItem:
    break;

  case AboveItem:
    index = dst->getIndexInParent();
    dst = dst->getParent();
    break;

  case BelowItem:
    index = dst->getIndexInParent()+1;
    dst = dst->getParent();
    break;

  case OnViewport:
    dst = nullptr;
    break; //ignored
  }

  //effect of removing src before inserting into dst
  if (index>=0 && src->getParent() == dst && index > src->getIndexInParent())
    --index;

  emit moveNodeRequest(dst, src, index);

  indicator.rect = QRect();
  indicator.pos = OnViewport;
  repaint();
}

/////////////////////////////////////////////////////////////////////////
void DataflowTreeView::paintEvent(QPaintEvent * evt)
{
  QPainter painter(viewport());
  drawTree(&painter, evt->region());

  //see https://github.com/jimmykuu/PyQt-PySide-Cookbook/blob/master/tree/drop_indicator.md
  if (indicator.rect.width() || indicator.rect.height())
  {
    painter.setPen(QPen(QBrush(QColor(Qt::black)), 2, Qt::SolidLine));
    if (indicator.rect.height() == 0)
      painter.drawLine(indicator.rect.topLeft(), indicator.rect.topRight());
    else
      painter.drawRect(indicator.rect);
  }
}

} //namespace Visus


