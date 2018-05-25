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

#include <Visus/Viewer.h>

#include <Visus/File.h>

#include <Visus/DatasetNode.h>
#include <Visus/KdQueryNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/TimeNode.h>
#include <Visus/ScriptingNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/StatisticsNode.h>
#include <Visus/PaletteNode.h>
#include <Visus/ModelViewNode.h>
#include <Visus/FieldNode.h>
#include <Visus/CpuPaletteNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/JTreeRenderNode.h>

#include <Visus/QueryNodeView.h>
#include <Visus/FreeTransformView.h>
#include <Visus/DatasetNodeView.h>
#include <Visus/StatisticsNodeView.h>
#include <Visus/TimeNodeView.h>
#include <Visus/GLCameraNodeView.h>
#include <Visus/FieldNodeView.h>
#include <Visus/ScriptingNodeView.h>
#include <Visus/CpuTransferFunctionNodeView.h>
#include <Visus/PaletteNodeView.h>
#include <Visus/RenderArrayNodeView.h>
#include <Visus/IsoContourNodeView.h>
#include <Visus/IsoContourRenderNodeView.h>
#include <Visus/VoxelScoopNodeView.h>
#include <Visus/JTreeNodeView.h>
#include <Visus/JTreeRenderNodeView.h>

#include <Visus/GLOrthoCamera.h>

#include <QInputDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QScreen>


namespace Visus {

///////////////////////////////////////////////////////////
void Viewer::postRedisplay()
{
  if (widgets.glcanvas)
    widgets.glcanvas->postRedisplay();
}

////////////////////////////////////////////////////////////
void Viewer::setPreferences(Preferences value)
{
  //for debugging I prefer to have always title bar and menus (example: debugging powerwall)
#ifdef _DEBUG
  value.bHideMenus   =false;
  value.bHideTitleBar=false;
#endif
  
  value.bRightHanded = true;
  this->preferences =value;

  auto dataflow=this->dataflow;
  setDataflow(SharedPtr<Dataflow>());
  setDataflow(dataflow);
}

////////////////////////////////////////////////////////////
void Viewer::createToolBar()
{
  auto& toolbar = widgets.toolbar;
  toolbar=new ToolBar();

  //MAIN tab
  {
    auto tab=toolbar->addTab("MAIN");

    tab->addBlueMenu(QIcon(), "File", toolbar->file_menu=GuiFactory::CreateMenu(this,{
      actions.New,
      actions.OpenFile,
      actions.AddFile,
      actions.SaveFile,
      actions.SaveFileAs,
      actions.SaveSceneAs,
      actions.SaveHistoryAs,
      actions.OpenUrl,
      actions.AddUrl,
      actions.ReloadVisusConfig,
      actions.Close }));
    
    tab->addAction(actions.EditNode);
    tab->addAction(actions.RenameNode);

    tab->addAction(actions.ShowHideNode);
    tab->addAction(actions.RemoveNode);

    tab->addAction(actions.Undo);
    tab->addAction(actions.Redo);

    tab->addAction(actions.Deselect);

    tab->addAction(actions.RefreshData);
    tab->addWidget(toolbar->auto_refresh.check = GuiFactory::CreateCheckBox(false, "Auto refresh", [this](int value) {
      auto auto_refresh = getAutoRefresh();
      auto_refresh.bEnabled = value ? true : false;
      setAutoRefresh(auto_refresh);

    }));
    tab->addWidget(toolbar->auto_refresh.msec = GuiFactory::CreateIntegerTextBoxWidget(0, [this](int value) {
      auto auto_refresh = getAutoRefresh();
      auto_refresh.msec = value ;
      setAutoRefresh(auto_refresh);
    }));
    toolbar->auto_refresh.msec->setMaxLength(4);
    toolbar->auto_refresh.msec->setMaximumWidth(50);
    toolbar->auto_refresh.msec->setFixedWidth(50);

    tab->addAction(actions.DropProcessing);

    tab->addBlueMenu(QIcon(":/zoom_fit.png"), "Fit", GuiFactory::CreateMenu(this,{
      actions.FitBest,
      actions.CameraX,
      actions.CameraY,
      actions.CameraZ
    }));

    tab->addBlueMenu(QIcon(":/snapshot.png"), "Snapshot", GuiFactory::CreateMenu(this,{
      actions.WindowSnapShot,
      actions.CanvasSnapShot
    }));

    tab->addAction(actions.MirrorX);
    tab->addAction(actions.MirrorY);

    tab->addBlueMenu(QIcon(),"ADD",GuiFactory::CreateMenu(this,{
      actions.AddGroup, 
      actions.AddTransform, 
      actions.InsertTransform, 
      actions.AddSlice, 
      actions.AddVolume, 
      actions.AddIsoContour, 
      actions.AddKdQuery, 
      actions.AddRender, 
      actions.AddKdRender, 
      actions.AddScripting, 
      actions.AddStatistics, 
      actions.AddCpuTransferFunction
    })); 

    toolbar->bookmarks_button=tab->addBlueMenu(QIcon(""), "Bookmarks", createBookmarks());

    tab->addAction(actions.ShowLicences);
    tab->addStretch(1);
  }

  addToolBar(toolbar);
}

////////////////////////////////////////////////////////////
void Viewer::createActions()
{
  addAction(actions.New = GuiFactory::CreateAction("New", this,[this]() {
    New();
  }));

  addAction(actions.OpenFile = GuiFactory::CreateAction("Open file...", this, [this]() {
    bool bAdd=false;
    openFile("", nullptr, bAdd); 
  }));

  addAction(actions.SaveFile = GuiFactory::CreateAction("Save",this, [this]() {
    saveFile(last_saved_filename);
  }));
  actions.SaveFile->setEnabled(!last_saved_filename.empty());
  actions.SaveFile->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
  actions.SaveFile->setToolTip("Save [CTRL+S]");

  addAction(actions.SaveFileAs = GuiFactory::CreateAction("Save as...",this, [this]() {
    saveFile(""); 
  }));
  
  addAction(actions.SaveSceneAs = GuiFactory::CreateAction("Export scene as...",this, [this]() {
    saveScene("");
  }));
  
  addAction(actions.SaveHistoryAs = GuiFactory::CreateAction("Save history as...",this, [this]() {
    bool bSaveHistory=true;
    saveFile("",bSaveHistory); 
  }));

  addAction(actions.AddFile = GuiFactory::CreateAction("Add file...", this, [this]() {
    auto parent=getRoot();
    bool bUrl=false;
    openFile("",parent,bUrl); 
  }));

  addAction(actions.OpenUrl = GuiFactory::CreateAction("Open url...", this, [this]() {
    Node* parent=nullptr;
    bool bUrl=true;
    openFile("", parent, bUrl); 
  }));

  addAction(actions.AddUrl = GuiFactory::CreateAction("Add url...", this, [this]() {
    auto parent=getRoot();
    bool bUrl=true;
    openFile("", parent, true); 
  }));

  addAction(actions.ReloadVisusConfig = GuiFactory::CreateAction("Reload config", this, [this]() {
    reloadVisusConfig();
  }));

  addAction(actions.Close = GuiFactory::CreateAction("Close", this, QIcon(":/quit.png"),[this]() {
    close();
  }));

  addAction(actions.RefreshData= GuiFactory::CreateAction("Refresh data",this, QIcon(":/refresh.png"), [this]() {
    refreshData();
  }));
  actions.RefreshData->setShortcut(Qt::Key_F5);
  actions.RefreshData->setToolTip("Refresh data [F5]");
 
  addAction(actions.DropProcessing= GuiFactory::CreateAction("Drop processing",this, QIcon(":/stop.png"), [this]() {
    dropProcessing();
  }));
  actions.DropProcessing->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5));
  actions.DropProcessing->setToolTip("Drop processing [CTRL+F5]");
 
  addAction(actions.WindowSnapShot= GuiFactory::CreateAction("Take Window snapshot",this,[this](){
    takeSnapshot(false);
  }));
  
  addAction(actions.CanvasSnapShot= GuiFactory::CreateAction("Take GLCanvas snapshot",this,[this](){
    takeSnapshot(true);
  }));
  
  addAction(actions.MirrorX= GuiFactory::CreateAction("Mirror X",this, QIcon(":/mirrorx.png"), [this]() {
    mirrorGLCamera(0);
  }));

  addAction(actions.MirrorY= GuiFactory::CreateAction("Mirror Y",this, QIcon(":/mirrory.png"), [this]() {
    mirrorGLCamera(1);
  }));

 addAction(actions.FitBest= GuiFactory::CreateAction("Best",this, [this]() {
   guessGLCameraPosition(-1);
  }));

  addAction(actions.CameraX= GuiFactory::CreateAction("X axis",this, [this]() {
    guessGLCameraPosition(0);
  }));

  addAction(actions.CameraY= GuiFactory::CreateAction("Y axis",this,[this](){
    guessGLCameraPosition(1);
  }));

  addAction(actions.CameraZ= GuiFactory::CreateAction("Z axis",this,[this](){
    guessGLCameraPosition(2);
  }));

  addAction(actions.EditNode= GuiFactory::CreateAction("Edit node",this, QIcon(":/edit.png"), [this]() {
    auto selection = getSelection();
    if (!selection) return;
    editNode();
  }));

  addAction(actions.Undo=GuiFactory::CreateAction("Undo",this, QIcon(":/undo.png"), [this]() {
    undo(); 
  }));
  actions.Undo->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z));
  actions.Undo->setToolTip("Undo [CTRL+Z]");

  addAction(actions.Redo=GuiFactory::CreateAction("Redo",this, QIcon(":/redo.png"), [this]() {
    redo(); 
  }));
  actions.Redo->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Y));
  actions.Redo->setToolTip("Redo [CTRL+Y]");

  addAction(actions.Deselect=GuiFactory::CreateAction("Deselect",this, QIcon(":/deselect.png"), [this]() {
    dropSelection();
  }));

  actions.Deselect->setShortcut(QKeySequence(Qt::Key_Escape));
  actions.Deselect->setToolTip("Deselect [ESC]");

  addAction(actions.RemoveNode=GuiFactory::CreateAction("Remove Node",this, QIcon(":/trash.png"), [this]() {
    auto selection = getSelection();
    if (!selection) return;
    removeNode(selection);
  }));
  actions.RemoveNode->setShortcut(QKeySequence(Qt::Key_Delete));
  actions.RemoveNode->setToolTip("Remove Node [DEL]");

  addAction(actions.RenameNode=GuiFactory::CreateAction("Rename Node",this, QIcon(":/rename.png"), [this]()
  {
    auto selection = getSelection();
    if (!selection) return;

    String name = cstring(QInputDialog::getText(this, "Insert the name:", "", QLineEdit::Normal, selection->getName().c_str()));
    if (name.empty()) return;
    setName(selection, name);
  }));

  addAction(actions.ShowHideNode=GuiFactory::CreateAction("Hide node",this, QIcon(":/eye.png"), [this]() {
    auto selection = getSelection();
    if (!selection) return;
    setHidden(selection, !selection->isHidden());
  }));

  addAction(actions.AddGroup=GuiFactory::CreateAction("Add Group",this, QIcon(":/group.png"), [this]() {
    auto selection = getSelection();
    if (!selection) return;
    addGroupNode(selection);
  }));

  addAction(actions.AddTransform=GuiFactory::CreateAction("Add Transform",this, QIcon(":/move.png"), [this]() {
    auto selection = getSelection();
    if (!selection) return;
    addModelViewNode(selection);
  }));

  addAction(actions.InsertTransform=GuiFactory::CreateAction("Insert transform",this, QIcon(":/move.png"), [this]() {
    auto selection = getSelection();
    if (!selection) return;
    addModelViewNode(selection,/*bInsert*/true);
  }));

  addAction(actions.AddSlice=GuiFactory::CreateAction("Add Slice",this, QIcon(":/slice.png"), [this]() {
    auto dataset_node = dynamic_cast<DatasetNode*>(getSelection());
    if (!dataset_node) return;
    int querydim = 2;
    String render = "Slice";
    addQueryNode(dataset_node, dataset_node, dataset_node->guessUniqueChildName("Slice"), querydim, "", 0, render);
  }));

  addAction(actions.AddVolume=GuiFactory::CreateAction("Add Volume",this, QIcon(":/volume.png"), [this]() {
    auto dataset_node = dynamic_cast<DatasetNode*>(getSelection());
    if (!dataset_node) return;
    int querydim = 3;
    String render = "Volume";
    addQueryNode(dataset_node, dataset_node, dataset_node->guessUniqueChildName("Volume"), querydim, "", 0, render);
  }));

  addAction(actions.AddIsoContour=GuiFactory::CreateAction("Add IsoContour",this, QIcon(":/mesh.png"), [this]() {

    auto selection = getSelection();
    if (!selection) return;
    if (auto dataset_node = dynamic_cast<DatasetNode*>(selection))
    {
      int querydim = 3;
      String render = "IsoContour";
      addQueryNode(dataset_node, dataset_node, dataset_node->guessUniqueChildName("IsoContour"), querydim, "", 0, render);
    }
    else if (selection && selection->hasOutputPort("data"))
    {
      addIsoContourNode(selection, selection);
    }
  }));

  addAction(actions.AddKdQuery=GuiFactory::CreateAction("Add KdQuery",this, QIcon(":/grid.png"), [this]() {
    auto dataset_node = dynamic_cast<DatasetNode*>(getSelection());
    if (!dataset_node) return;
    addKdQueryNode(dataset_node, dataset_node);
  }));


  addAction(actions.AddKdRender=GuiFactory::CreateAction("Add KdRender",this, QIcon(":/paint.png"), [this]() {
    KdQueryNode* kdquery = dynamic_cast<KdQueryNode*>(getSelection());
    if (!kdquery) return;
    addKdRenderArrayNode(kdquery, kdquery);
  }));
 
  addAction(actions.AddRender=GuiFactory::CreateAction("Add Render",this, QIcon(":/paint.png"), [this]() {
    auto selection = getSelection();
    if (!selection || !selection->hasOutputPort("data")) return;
    addRenderArrayNode(selection, selection);
  }));

  addAction(actions.AddScripting=GuiFactory::CreateAction("Add Scripting",this, QIcon(":/cpu.png"), [this]() {
    auto selection = getSelection();
    if (!selection || !selection->hasOutputPort("data")) return;
    addScriptingNode(selection, selection);
  }));

  addAction(actions.AddStatistics=GuiFactory::CreateAction("Add Statistics",this, QIcon(":/statistics.png"), [this]() {
    auto selection = getSelection();
    if (!selection || !selection->hasOutputPort("data")) return;
    addStatisticsNode(selection, selection);
  }));

  addAction(actions.AddCpuTransferFunction=GuiFactory::CreateAction("Add CpuTransf",this, QIcon(":/cpu.png"), [this]() {
    auto selection = getSelection();
    if (!selection || !selection->hasOutputPort("data")) return;
    addCpuTransferFunctionNode(selection, selection);
  }));

  addAction(actions.ShowLicences = GuiFactory::CreateAction("Licences...", this, [this]() {
    showLicences();
  }));
}

////////////////////////////////////////////////////////////
void Viewer::createBookmarks(QMenu* dst,const StringTree* src)
{
  for (int I=0;I<(int)src->getNumberOfChilds();I++)
  {
    const StringTree* child=&src->getChild(I);

    if (child->name=="dataset")
    {
      //NOTE: i'm using name and not url because I can have multiple dataset with the same url (example: cached, no cached)
      //      later loadDataset (with DatasetGetInfo) will figure out the real url
      String url=child->readString("name",child->readString("url"));
      VisusAssert(!url.empty());
      dst->addAction(GuiFactory::CreateAction(StringUtils::replaceAll(url, "&", "&&").c_str(), this, [this, url]() {
        openFile(url); 
      }));
    }
    else if (child->name == "scene")
    {
      String name = child->readString("name"); VisusAssert(!name.empty());
      String url   = child->readString("url"); VisusAssert(!url.empty() && (StringUtils::endsWith(url, ".xml") || StringUtils::endsWith(url, ".scn")));
      name = StringUtils::replaceAll(name, "&", "&&");

      dst->addAction(GuiFactory::CreateAction(name.c_str(), this, [this, url]() {
        openFile(url);
      }));
    }
    else if (child->name=="group")
    {
      QMenu* submenu=dst->addMenu(child->readString("name",child->name).c_str());
      createBookmarks(submenu,child);
    }
    else
    {
      createBookmarks(dst,child);
    }
  }
}

//////////////////////////////////////////////////////////////////////
DataflowTreeView* Viewer::createTreeView()
{
  auto ret=new DataflowTreeView(this->dataflow.get());
  
  QPalette p(ret->palette());
  p.setColor(QPalette::Base, Qt::darkGray);
  ret->setPalette(p);

  ret->getIcon=[this](Node* node) 
  {
    if (!node || !dataflow || node==getRoot()) 
      return icons->world;

    if (dynamic_cast<GLCameraNode*>(node))
      return icons->camera;

    if (dynamic_cast<TimeNode*>(node))
      return icons->clock;

    if (dynamic_cast<ScriptingNode*>(node))
      return icons->cpu;

    if (dynamic_cast<DatasetNode*>(node))
      return icons->dataset;

    if (dynamic_cast<QueryNode*>(node))
      return icons->gear;

    if (dynamic_cast<RenderArrayNode*>(node))
      return icons->paint;

    if (dynamic_cast<StatisticsNode*>(node))
      return icons->statistics;

    if (dynamic_cast<PaletteNode*>(node))
      return icons->palette;

    if (dynamic_cast<JTreeRenderNode*>(node))
      return icons->brush;

    if (node->getChilds().empty())
      return icons->document;

    return icons->group;
  };

  connect(ret,&QTreeWidget::itemClicked,[this](QTreeWidgetItem* widget,int){
    if (!widgets.treeview) return;
    if (Node* node=widgets.treeview->getNode(widget)) 
      setSelection(node);
  });

  connect(ret,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem* widget,int) {
    if (!widgets.treeview) return;
    if (Node* node=widgets.treeview->getNode(widget)) 
      editNode(node);
  });

  connect(ret,&QTreeWidget::itemSelectionChanged,this,[this]() {
    if (!widgets.treeview) return;
    Node* selection=widgets.treeview->selectedItems().empty()? nullptr : widgets.treeview->getNode(widgets.treeview->selectedItems()[0]);
    setSelection(selection);
  });

  connect(ret,&DataflowTreeView::moveNodeRequest,[this](Node* dst,Node* src,int index) {

    if (!src || !dst)
      return;

    moveNode(dst,src,index);
  });

  ret->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ret, &QTreeWidget::customContextMenuRequested, [this, ret](const QPoint & pos) {

    QTreeWidgetItem *widget = ret->itemAt(pos);
    auto node = ret->getNode(widget);
    if (!node)
      return;

    QMenu menu(this);

    for (auto action : { actions.EditNode ,actions.RemoveNode,actions.RenameNode,actions.ShowHideNode,actions.Deselect })
    {
      if (action && action->isEnabled())
        menu.addAction(action);
    }

    menu.addSeparator();

    for (auto action : {
      actions.AddGroup,
      actions.AddTransform, 
      actions.InsertTransform,
      actions.AddSlice,
      actions.AddVolume,
      actions.AddIsoContour,
      actions.AddKdQuery,
      actions.AddRender,
      actions.AddKdRender,
      actions.AddScripting,
      actions.AddStatistics,
      actions.AddCpuTransferFunction 
    })
    {
      if (action && action->isEnabled())
        menu.addAction(action);
    }

    menu.addSeparator();

    if (dynamic_cast<GLCameraNode*>(node))
    {
      for (auto action : { actions.MirrorX ,actions.MirrorY,actions.FitBest,actions.CameraX,actions.CameraY,actions.CameraZ })
      {
        if (action && action->isEnabled())
          menu.addAction(action);
      }
    }

    if (menu.actions().size()==0)
      return;

    menu.exec(ret->mapToGlobal(pos));

  });

  return ret;
}

////////////////////////////////////////////////////////////
void Viewer::showLicences()
{
  String content="See Copyrights/ directory";

  auto layout = new QVBoxLayout();

  auto text=GuiFactory::CreateTextEdit();
  text->setPlainText(content.c_str());
  layout->addWidget(text);

  auto buttons = new QDialogButtonBox();
  auto ok_button=new QPushButton(tr("Ok"));
  ok_button->setDefault(true);
    
  buttons->addButton(ok_button,QDialogButtonBox::AcceptRole);
  layout->addWidget(buttons);

  auto dialog = new QDialog();
  dialog->resize(QSize(480, 640));
  dialog->setLayout(layout);

  connect(ok_button, &QPushButton::clicked, [dialog]() {
    dialog->close();
  });

  dialog->show();
}

////////////////////////////////////////////////////////////
void Viewer::refreshActions()
{
  auto selection=getSelection();

  bool b2D = std::dynamic_pointer_cast<GLOrthoCamera>(getGLCamera()) ? true : false;
  actions.MirrorX->setEnabled(b2D);
  actions.MirrorY->setEnabled(b2D);

  actions.EditNode->setEnabled(selection?true:false);
  actions.RemoveNode->setEnabled(selection && selection!=getRoot());
  actions.RenameNode->setEnabled(selection?true:false);

  actions.ShowHideNode->setEnabled(selection?true:false);
  actions.ShowHideNode->setText(selection && selection->isHidden()? "Show node" : "Hide Node");
  
  actions.Undo->setEnabled(canUndo());
  actions.Redo->setEnabled(canRedo());
  actions.Deselect->setEnabled(selection?true:false);

  actions.AddGroup->setEnabled(selection?true:false);
  actions.AddTransform->setEnabled(selection?true:false);
  actions.InsertTransform->setEnabled(selection && selection!=getRoot());
  actions.AddSlice->setEnabled(selection && dynamic_cast<DatasetNode*>(selection));
  actions.AddVolume->setEnabled(selection && dynamic_cast<DatasetNode*>(selection));
  actions.AddIsoContour->setEnabled(selection && (dynamic_cast<DatasetNode*>(selection) || selection->hasOutputPort("data")));
  actions.AddKdQuery->setEnabled(selection && dynamic_cast<DatasetNode*>(selection));
  actions.AddRender->setEnabled(selection && selection->hasOutputPort("data"));
  actions.AddKdRender->setEnabled(selection && dynamic_cast<KdQueryMode*>(selection));
  actions.AddScripting->setEnabled(selection && selection->hasOutputPort("data"));
  actions.AddStatistics->setEnabled(selection && selection->hasOutputPort("data"));
  actions.AddCpuTransferFunction->setEnabled(selection && selection->hasOutputPort("data"));
}

////////////////////////////////////////////////////////////////////////////////
void Viewer::editNode(Node* node)
{
  if (!node)
    node=getSelection();

  //QueryNode
  if (auto query_node=dynamic_cast<QueryNode*>(node))
  {
    if (!widgets.glcanvas) 
      return;

    addDockWidget("QueryNode",new QueryNodeView(query_node));

    //this force the creation of the free_transform
    this->dropSelection();
    this->setSelection(query_node); 

    if (free_transform) 
    {
      auto view=new FreeTransformView(free_transform.get());
      addDockWidget("FreeTransform",view);
    }

    return;
  }

  //ModelViewNode
  if (auto model=dynamic_cast<ModelViewNode*>(node))
  {
    if (!widgets.glcanvas) 
      return;

    //this force the creation of the free_transform
    this->dropSelection();
    this->setSelection(model);

    if (free_transform)
    {
      auto view=new FreeTransformView(free_transform.get());
      view->widgets.box->setEnabled(false); //I cannot edit the down bounds, I can change only the matrix
      addDockWidget("FreeTransform",view);
    }

    return ;
  }

  //DatasetNode
  if (auto model=dynamic_cast<DatasetNode*>(node))
  {
    addDockWidget("Dataset Node",new DatasetNodeView(model));
    return;
  }

  //TimeNode
  if (auto model=dynamic_cast<TimeNode*>(node))
  {
    addDockWidget("Time Node",new TimeNodeView(model));
    return;
  }

  //GLCameraNode
  if (auto model=dynamic_cast<GLCameraNode*>(node))
  {
    addDockWidget("GLCamera Node",new GLCameraNodeView(model));
    return;
  }

  //StatisticsNode
  if (auto model=dynamic_cast<StatisticsNode*>(node))
  {
    addDockWidget("Statistics Node",new StatisticsNodeView(model));
    return;
  }

  //FieldNode
  if (auto field_node=dynamic_cast<FieldNode*>(node))
  {
    //find the dataset...
    SharedPtr<Dataset> dataset;
    if (QueryNode* query=field_node->getOutputPort("fieldname")->findFirstConnectedOutputOfType<QueryNode*>())
    {
      if (DatasetNode* dataset_node=query->getInputPort("dataset")->findFirstConnectedInputOfType<DatasetNode*>())
        dataset=dataset_node->getDataset();
    }

    auto widget=new FieldNodeView(field_node,dataset);

    if (std::dynamic_pointer_cast<IdxMultipleDataset>(dataset))
      addDockWidget(node->getName(),widget);
    else
      showPopupWidget(new FieldNodeView(field_node,dataset));
    return;
  }

  //ScriptingNode
  if (auto model=dynamic_cast<ScriptingNode*>(node))
  {
    addDockWidget(node->getName(),new ScriptingNodeView(model));
    return;
  }

 //CpuPaletteNode
  if (auto model=dynamic_cast<CpuPaletteNode*>(node))
  {
    addDockWidget(node->getName(),new CpuTransferFunctionNodeView(model));
    return;
  }

  //PaletteNode
  if (auto model=dynamic_cast<PaletteNode*>(node))
  {
    addDockWidget(node->getName(),new PaletteNodeView(model));
    return;
  }

  //RenderArrayNode
  if (auto model=dynamic_cast<RenderArrayNode*>(node))
  {
    addDockWidget(node->getName(),new RenderArrayNodeView(model));
    return;
  }

  //IsoContourNode
  if (auto model=dynamic_cast<IsoContourNode*>(node))
  {
    addDockWidget(node->getName(),new IsoContourNodeView(model));
    return;
  }

  //IsoContourRenderNode
  if (auto model=dynamic_cast<IsoContourRenderNode*>(node))
  {
    addDockWidget(node->getName(),new IsoContourRenderNodeView(model));
    return;
  }

  //VoxelScoopNode
  if (auto model=dynamic_cast<VoxelScoopNode*>(node))
  {
    addDockWidget("VoxelScoop",new VoxelScoopNodeView(model));
    return;
  }

  //JTreeNode
  if (auto model=dynamic_cast<JTreeNode*>(node))
  {
    addDockWidget("JTreeNode",new JTreeNodeView(model));
    return;
  }

  //JTreeRenderNode
  if (auto model=dynamic_cast<JTreeRenderNode*>(node))
  {
    addDockWidget("JTreeRenderNode",new JTreeRenderNodeView(model));
    return;
  }
  
}

///////////////////////////////////////////////////////////
void Viewer::showPopupWidget(QWidget* widget)
{
  auto layout=new QVBoxLayout();
  layout->addWidget(widget);

  auto popup = new QWidget(this, Qt::Popup);
  popup->setLayout(layout);
  popup->move(QCursor::pos().x(),QCursor::pos().y());
  popup->show();

}

///////////////////////////////////////////////////////////
void Viewer::addDockWidget(String name,QWidget* widget)
{
  auto layout=new QVBoxLayout();
  layout->addWidget(new QLabel(name.c_str()));
  layout->addWidget(widget);
  //layout->addLayout(new QHBoxLayout(),1); //filler

  auto frame=new QFrame();
  frame->setLayout(layout);
  auto dock = new QDockWidget(name.c_str(),this);
  dock->setWidget(frame);
  addDockWidget(Qt::RightDockWidgetArea, dock);
}

////////////////////////////////////////////////////////////
void Viewer::keyPressEvent(QKeyEvent* evt)
{
  if (auto glcamera = getGLCamera())
    glcamera->glKeyPressEvent(evt);
}


//////////////////////////////////////////////////////////////////////
bool Viewer::takeSnapshot(bool bOnlyCanvas,String filename)
{
  //gues the filename
  if (filename.empty())
  {
    for (int I=0;;I++)
    {
      filename = KnownPaths::VisusHome.getChild(StringUtils::format() << "visus_snapshot." << std::setfill('0') << std::setw(3) << cstring(I) << ".png");
      if (!FileUtils::existsFile(filename))
        break;
    }
  }

  if (bOnlyCanvas)
  {
    auto frame_buffer = widgets.glcanvas->grabFramebuffer();
    if (!frame_buffer.width() || !frame_buffer.height())
    {
      VisusWarning() << "Failed to grabFramebuffer";
      return false;
    }
    
    if (!frame_buffer.save(filename.c_str(), "PNG"))
    {
      VisusWarning() << "Failed to save filename " << filename;
      return false;
    }
  }
  else 
  {
    if (qApp->screens().size()!= 1)
    {
      VisusWarning() << "Multiple screens snapshot is not supported";
      return false;
    }

    auto main_screen = qApp->primaryScreen();
    if (!main_screen)
    {
      VisusWarning() << "Primary screen does not exist";
      return false;
    }

    auto pixmap = main_screen->grabWindow(this->winId());
    if (!pixmap.width() || !pixmap.height())
    {
      VisusWarning() << "Failed to grabWindow";
      return false;
    }

    if (!pixmap.save(filename.c_str(), "PNG"))
    {
      VisusWarning() << "Failed to save filename " << filename;
      return false;
    }
  }

  VisusInfo() << "Saved snapshot " << filename;
  return true;
}


} //namespace Visus

