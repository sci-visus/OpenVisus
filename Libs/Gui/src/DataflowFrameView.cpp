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

#include <Visus/Gui.h>
#include <Visus/DataflowFrameView.h>

#include <QLabel>
#include <QMouseEvent>
#include <QPainterPath>

namespace Visus {


////////////////////////////////////////////////////////////////
class DataflowFrameView::NodeWidget : public QFrame
{
public:

  DataflowFrameView* owner;
  Node* node;

  //constructor
  NodeWidget(DataflowFrameView* owner_,Node* node_) 
    : owner(owner_),node(node_),titlebar(20),border(5),bDragging(false),mouse_cursor(Default),title_color(getDefaultTitleColor())
  {
  }

  //destructor
  virtual ~NodeWidget()
  {
    setWidget(nullptr);
  }

  //getTitle
  String getTitle() const
  {
    return this->title;
  }

  //setTitle
  void setTitle(String value)
  {
    this->title = value; this->update();
  }

  //getTitleColor
  Color getTitleColor() const
  {
    return title_color;
  }

  //setTitleColor
  void setTitleColor(Color value)
  {
    this->title_color = value; this->update();
  }

  //getDefaultTitleColor
  static Color getDefaultTitleColor()
  {
    return Colors::DarkBlue;
  }

  //getWidget 
  QWidget* getWidget()
  {
    return widget;
  }

  //setWidget
  void setWidget(QWidget* VISUS_DISOWN(value))
  {
    if (widget)
    {
      widget->setVisible(false);
      widget->setParent(nullptr);
      delete widget;
      widget = nullptr;
    }

    this->widget = value;

    if (this->widget)
    {
      widget->setParent(this);
      widget->setVisible(true);
    }

    refreshComponentBounds();
  }

  //resizeEvent
  virtual void resizeEvent(QResizeEvent* evt) override
  {
    QFrame::resizeEvent(evt);
    refreshComponentBounds();

    node->frameview_bounds = owner->normalizedGeometry(this);
    if (owner->elastic_layout.enabled && owner->elastic_layout.dragging == node)
      owner->setElasticDraggingNode(node);
    owner->update();

  }

  //moveEvent
  virtual void moveEvent(QMoveEvent* evt) override
  {
    QFrame::moveEvent(evt);
    refreshComponentBounds();

    node->frameview_bounds = owner->normalizedGeometry(this);
    if (owner->elastic_layout.enabled && owner->elastic_layout.dragging == node)
      owner->setElasticDraggingNode(node);
    owner->update();
  }

  //showEvent
  virtual void showEvent(QShowEvent *evt) override
  {
    QFrame::showEvent(evt);
    refreshComponentBounds();
  }

  //hideEvent
  virtual void hideEvent(QHideEvent *evt) override
  {
    QFrame::hideEvent(evt);
    refreshComponentBounds();
  }


  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override {
    QFrame::mousePressEvent(evt);
    if (evt->button() != Qt::LeftButton) return;
    updateMouseCursor(evt->x(), evt->y());
    this->original_bounds = QUtils::convert<Rectangle2d>(geometry());
    this->dragging_start = QUtils::convert<Point2i>(mapToGlobal(QPoint(0, 0))) + Point2i(evt->x(), evt->y());
    this->bDragging = (evt->x() >= 0 && evt->x() < width() && evt->y() >= 0 && evt->y() < titlebar);

    if (owner->elastic_layout.enabled)
      owner->setElasticDraggingNode(node);
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override {
    QFrame::mouseMoveEvent(evt);

    int button =
      (evt->buttons() & Qt::LeftButton) ? Qt::LeftButton :
      (evt->buttons() & Qt::RightButton) ? Qt::RightButton :
      (evt->buttons() & Qt::MiddleButton) ? Qt::MiddleButton :
      0;

    if (button != Qt::LeftButton)
    {
      updateMouseCursor((int)evt->x(), (int)evt->y());
      return;
    }

    Point2i delta = (QUtils::convert<Point2i>(mapToGlobal(QPoint(0, 0))) + Point2i(evt->x(), evt->y())) - dragging_start;

    double X = original_bounds.x;
    double Y = original_bounds.y;
    double W = original_bounds.width;
    double H = original_bounds.height;

    if (bDragging)
    {
      X += delta[0];
      Y += delta[1];
    }
    else
    {
      if (mouse_cursor & Left) X += delta[0];
      if (mouse_cursor & Top) Y += delta[1];
      if (mouse_cursor & Right) W += delta[0];
      if (mouse_cursor & Bottom) H += delta[1];
    }

    this->setGeometry((int)X, (int)Y, (int)W, (int)H);

    if (owner->elastic_layout.enabled)
      owner->setElasticDraggingNode(node);
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override {
    QFrame::mouseReleaseEvent(evt);
    this->bDragging = false;
    this->mouse_cursor = Default;
  }


private:

  VISUS_NON_COPYABLE_CLASS(NodeWidget)

    //title
    String title;

  //title_color
  Color title_color;

  enum
  {
    Default = 0x00,
    Center = 0x01,
    Left = 0x02,
    Top = 0x04,
    Right = 0x08,
    Bottom = 0x10
  };

  QWidget* widget = nullptr;

  bool        bDragging;
  int         mouse_cursor;
  Point2i     dragging_start;
  int         titlebar;
  int         border;
  Rectangle2d original_bounds;

  //refreshComponentBounds
  void refreshComponentBounds()
  {
    if (!widget || !this->isVisible())
      return;

    widget->setGeometry(border, titlebar, width() - border * 2, height() - titlebar - border);
  }


  //updateMouseCursor
  void updateMouseCursor(int x, int y)
  {
    int new_mouse_cursor = Default;

    int w = (int)width();
    int h = (int)height();

    if (bool bInside = (x <= border || x >= (w - border) || y <= border || y >= (h - border)))
    {
      if (x <= border) new_mouse_cursor |= Left;
      else if (x >= (w - border)) new_mouse_cursor |= Right;
      if (y < border) new_mouse_cursor |= Top;
      else if (y >= (h - border)) new_mouse_cursor |= Bottom;
    }

    if (this->mouse_cursor != new_mouse_cursor)
    {
      this->mouse_cursor = new_mouse_cursor;
      switch (new_mouse_cursor)
      {
      case (Left | Top    ) : setCursor(Qt::SizeFDiagCursor); break;
      case (Top           ) : setCursor(Qt::SizeVerCursor  ); break;
      case (Right | Top   ) : setCursor(Qt::SizeBDiagCursor); break;
      case (Left          ) : setCursor(Qt::SizeHorCursor  ); break;
      case (Right         ) : setCursor(Qt::SizeHorCursor  ); break;
      case (Left | Bottom ) : setCursor(Qt::SizeBDiagCursor); break;
      case (Bottom        ) : setCursor(Qt::SizeVerCursor  ); break;
      case (Right | Bottom) : setCursor(Qt::SizeFDiagCursor); break;
      default:                setCursor(Qt::ArrowCursor    ); break;
      }
    }
  }

  //enterEvent
  virtual void enterEvent(QEvent* evt) override
  {
    updateMouseCursor(QCursor::pos().x(), QCursor::pos().y());
  }

  //paint
  virtual void paintEvent(QPaintEvent*) override
  {
    QPainter g(this);

    int w = (int)width();
    int h = (int)height();

    //draw the title bar
    {
      g.fillRect(0, 0, width(), titlebar, QUtils::convert<QColor>(title_color));
      g.setPen(QUtils::convert<QColor>(Colors::White));
      g.drawText(3, titlebar - 2, title.c_str());
    }

    //draw the resizer
    {
      g.setPen(QUtils::convert<QColor>(Colors::DarkGray));
      g.drawRect(0, 0, w, h);
      g.setPen(QUtils::convert<QColor>(Colors::DarkGray.withAlpha(0.1f)));
      g.drawRect(border - 1, border - 1, w - 2 * border + 2, h - 2 * border + 2);
    }
  }

};

/////////////////////////////////////////////////////////////////////////////////////
DataflowFrameView::DataflowFrameView(Dataflow* dataflow) 
{
  elastic_layout.enabled=true;
  elastic_layout.dragging=nullptr;
  connect(&elastic_layout.timer,&QTimer::timeout,[this](){computeElasticLayout();});
  setDataflow(dataflow);
}

/////////////////////////////////////////////////////////////////////////////////////
DataflowFrameView::~DataflowFrameView()
{
  setDataflow(nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::setDataflow(Dataflow* value)
{
  if (this->dataflow)
  {
    for (auto it=get_node.begin();it!=get_node.end();it++)
    {
      QWidget* widget=it->first;
      widget->setVisible(false);
      widget->setParent(nullptr);
      delete widget;
    }

    get_node  .clear();
    get_widget.clear();
    Utils::remove(this->dataflow->listeners,this);
  }

  this->dataflow=value;

  if (this->dataflow)
  {
    this->dataflow->listeners.push_back(this);
    elastic_layout.started=Time::now();

    for (auto node : dataflow->getNodes())
      addNode(node);
  }
}


/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::refreshBounds()
{
  if (!dataflow || !isVisible()) 
    return;

  for (auto node : dataflow->getNodes())
  {
    if (QWidget* widget =getWidget(node))
      setNormalizedGeometry(widget,node->frameview_bounds);
  }

}

/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::resizeEvent(QResizeEvent* evt)
{
  QFrame::resizeEvent(evt);
  refreshBounds();
}

/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::showEvent(QShowEvent *evt)
{
  QFrame::showEvent(evt);
  refreshBounds();
}

/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::hideEvent(QHideEvent *evt)
{
  QFrame::hideEvent(evt);
  refreshBounds();
}

/////////////////////////////////////////////////////////////////////////////////////
inline Rectangle2d DataflowFrameView::normalizedGeometry(QWidget* widget)
{
  double W=width(),H=height();
  if (!W || !H) return Rectangle2d();
  Rectangle2d ret=QUtils::convert<Rectangle2d>(widget->geometry());
  ret.x/=W;ret.width /=W;
  ret.y/=H;ret.height/=H;
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////
inline void DataflowFrameView::setNormalizedGeometry(QWidget* widget,Rectangle2d r)
{
  double W=width(),H=height();
  if (!W || !H) return ;
  r.x*=W;r.width *=W;
  r.y*=H;r.height*=H;
  widget->setGeometry(r.x,r.y,r.width,r.height);
}

/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::addNode(Node* node)
{
  VisusAssert(dataflow);
  VisusAssert(getWidget(node)==nullptr);

  NodeWidget* widget=new NodeWidget(this,node);
  widget->setTitle(node->getName());
  widget->setWidget(new QLabel(node->getTypeName().c_str()));

  Rectangle2d& r=node->frameview_bounds;
  if (!r.valid())
  {
    double S=sqrt(0.002*(std::max(1,(int)node->inputs.size()+(int)node->outputs.size())));
    r.width =S*1.5;
    r.height=S;
    r.x=Utils::getRandDouble(r.width ,1.0-r.width );
    r.y=Utils::getRandDouble(r.height,1.0-r.height);
  }

  setNormalizedGeometry(widget,r);

  widget->setParent(this);
  widget->setVisible(true);

  get_widget[node]=widget;
  get_node[widget]=node;
}


/////////////////////////////////////////////////////////////////////////////////////
void DataflowFrameView::setElasticDraggingNode(Node* node)
{
  if (node)
  {
    elastic_layout.dragging=node;
    elastic_layout.started=Time::now();
    elastic_layout.timer.stop ();
    elastic_layout.timer.start(1000/30);
  }
  else
  {
    elastic_layout.dragging=nullptr;
    elastic_layout.timer.stop ();
  }
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowSetName(Node* node,String old_value,String new_value) {
  if (auto floating=dynamic_cast<NodeWidget*>(getWidget(node)))
    floating->setTitle(new_value);
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowSetHidden(Node* node,bool ,bool ) {
  if (auto floating=dynamic_cast<NodeWidget*>(getWidget(node)))
    floating->setTitleColor(Colors::LightBlue);
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowAddNode(Node* node) {
  addNode(node);
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowRemoveNode(Node* node) {

  if (elastic_layout.enabled)
    setElasticDraggingNode(nullptr);

  QWidget* widget=getWidget(node);VisusAssert(widget);
  get_widget.erase(node);
  get_node.erase(widget);
  widget->setVisible(false);
  widget->setParent(nullptr);
  delete widget;
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowMoveNode(Node* dst,Node* src,int index) {
  update();
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowSetSelection(Node* old_selection,Node* new_selection) {

  if (auto floating=dynamic_cast<NodeWidget*>(getWidget(old_selection)))
    floating->setTitleColor(NodeWidget::getDefaultTitleColor());

  if (auto floating=dynamic_cast<NodeWidget*>(getWidget(new_selection)))
    floating->setTitleColor(Colors::Yellow);

}
/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowConnectNodes(Node* from,String oport,String iport,Node* to) {
  update();
}

/////////////////////////////////////////////////////////////
void DataflowFrameView::dataflowDisconnectNodes(Node* from,String oport,String iport,Node* to) {
  update();
}

/////////////////////////////////////////////////////////////
inline int getTotalNumberOfNodeConnections(Node* node)
{
  int ret=0;
  for (auto it=node->inputs .begin();it!=node->inputs .end();it++) ret+=(int)it->second->inputs.size() + (int)it->second->outputs.size();
  for (auto it=node->outputs.begin();it!=node->outputs.end();it++) ret+=(int)it->second->inputs.size() + (int)it->second->outputs.size();
  return ret;
}


/////////////////////////////////////////////////////////////
void DataflowFrameView::computeElasticLayout()
{
  if (elastic_layout.started.elapsedSec()>=3.0)
    return;

  //see also http://profs.etsmtl.ca/mmcguffin/research/2012-mcguffin-simpleNetVis/mcguffin-2012-simpleNetVis.pdf
  VisusAssert(elastic_layout.enabled && elastic_layout.dragging);
  Node* dragging=elastic_layout.dragging;

  std::vector<Node*> move_inputs;
  std::vector<Node*> move_outputs;
  for (auto it=dragging->inputs.begin();it!=dragging->inputs.end();it++)
  {
    DataflowPort* iport=it->second;for (auto jt=iport->inputs.begin();jt!=iport->inputs.end();jt++)
    {
      DataflowPort* oport=*jt;
      if (getTotalNumberOfNodeConnections(oport->getNode())==1) 
        move_inputs.push_back(oport->getNode());
    } 
  }

  for (auto it=dragging->outputs.begin();it!=dragging->outputs.end();it++)
  {
    DataflowPort* oport=it->second;for (auto jt=oport->outputs.begin();jt!=oport->outputs.end();jt++)
    {
      DataflowPort* iport=*jt;
      if (getTotalNumberOfNodeConnections(iport->getNode())==1) 
        move_outputs.push_back(iport->getNode());
    }
  }

  const double R=1.5*std::max(dragging->frameview_bounds.width,dragging->frameview_bounds.height);

  for (int N=0,M=(int)move_inputs.size();N<M;N++)
  {
    Node* input=move_inputs[N];

    Point2d p1 = input   ->frameview_bounds.center();
    Point2d p2 = dragging->frameview_bounds.center();

    double A=Math::Pi-Math::Pi/4;
    double B=Math::Pi+Math::Pi/4;
    double angle=A+(B-A)*(M>1?(N/(M-1.0)):0.5);
    p2=p2+Point2d(R*cos(angle),-R*sin(angle));

    double dx=p2[0]-p1[0];
    double dy=p2[1]-p1[1];
    double distance = sqrt(dx*dx+dy*dy);
    if (!distance) continue;
    double cos_alpha = dx / distance; input->frameview_bounds.x += 0.2 * distance * cos_alpha ;
    double sin_alpha = dy / distance; input->frameview_bounds.y += 0.2 * distance * sin_alpha ;

    if (QWidget* widget=getWidget(input))
      setNormalizedGeometry(widget,input->frameview_bounds);
  }

  for (int N=0,M=(int)move_outputs.size();N<M;N++)
  {
    Node* output=move_outputs[N];

    Point2d p1 = dragging->frameview_bounds.center();
    Point2d p2 = output  ->frameview_bounds.center();

    double A=+Math::Pi/4;
    double B=-Math::Pi/4;
    double angle=A+(B-A)*(M>1?(N/(M-1.0)):0.5);
    p1=p1+Point2d(R*cos(angle),R*sin(angle));

    double dx=p2[0]-p1[0];
    double dy=p2[1]-p1[1];
    double distance = sqrt(dx*dx+dy*dy);
    if (!distance) continue;
    double cos_alpha = dx / distance; output->frameview_bounds.x -= 0.2 * distance * cos_alpha ;
    double sin_alpha = dy / distance; output->frameview_bounds.y -= 0.2 * distance * sin_alpha ;

    if (QWidget* widget=getWidget(output))
      setNormalizedGeometry(widget,output->frameview_bounds);
  }

  update();
}

/////////////////////////////////////////////////////////////
Point2d DataflowFrameView::getInputPortPosition(DataflowPort* iport)
{
  Node* node=iport->getNode();
  Rectangle2d& r=node->frameview_bounds;
  int N=(int)std::distance(node->inputs.begin(),node->inputs.find(iport->getName()));
  double beta=(N+1)/(double)(node->inputs.size()+1);
  double W=width();
  double H=height();
  return Point2d(W*(r.x),H*(r.y+beta*r.height));
}

/////////////////////////////////////////////////////////////
Point2d DataflowFrameView::getOutputPortPosition(DataflowPort* oport)
{
  Node* node=oport->getNode();
  Rectangle2d& pos=node->frameview_bounds;
  int N=(int)std::distance(node->outputs.begin(),node->outputs.find(oport->getName()));
  double beta=(N+1)/(double)(node->outputs.size()+1);
  double W=width ();
  double H=height();
  return Point2d(W*(pos.x+pos.width),H*(pos.y+beta*pos.height));
}


/////////////////////////////////////////////////////////////
void DataflowFrameView::paintEvent(QPaintEvent* evt)
{  
  if (!dataflow) 
    return;

  QPainter g(this);

  for (auto from : dataflow->getNodes())
  {
    g.setOpacity(from==dataflow->getSelection()?1.0:0.5);

    //draw input port labels
    for (auto it=from->inputs.begin();it!=from->inputs.end();it++)
    {
      Point2d pos=getInputPortPosition(it->second);
      g.drawText(QPoint((int)pos[0],(int)pos[1])/*,Qt::AlignRight | Qt::AlignBottom*/,it->first.c_str());
    }

    //draw output port labels and lines
    for (auto it=from->outputs.begin();it!=from->outputs.end();it++)
    {
      Point2d oport_position=getOutputPortPosition(it->second);
      g.drawText(QPoint((int)oport_position[0],(int)oport_position[1])/*,Qt::AlignLeft | Qt::AlignBottom*/,it->first.c_str());

      for (auto jt=it->second->outputs.begin();jt!=it->second->outputs.end();jt++)
      {
        Point2d  iport_position = getInputPortPosition(*jt);

        Point2d p1=oport_position;
        Point2d p4=iport_position; double R=p1.distance(p4)/4.0;
        Point2d p2=p1+Point2d(R,0);
        Point2d p3=p4-Point2d(R,0);

        QPainterPath path;
        path.moveTo ((float)p1[0], (float)p1[1]);
        path.cubicTo((float)p2[0], (float)p2[1], (float)p3[0], (float)p3[1], (float)p4[0], (float)p4[1]);
        g.strokePath(path, g.pen());
      }
    }
  }
}

} //namespace Visus


