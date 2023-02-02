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

#ifndef VISUS_VIEWER_H
#define VISUS_VIEWER_H

#include <Visus/Gui.h>
#include <Visus/Model.h>
#include <Visus/FreeTransform.h>
#include <Visus/GLCamera.h>
#include <Visus/Dataset.h>
#include <Visus/GLCameraNode.h>
#include <Visus/DataflowFrameView.h>
#include <Visus/DataflowTreeView.h>
#include <Visus/Thread.h>
#include <Visus/NetSocket.h>
#include <Visus/StringTree.h>
#include <Visus/NetServer.h>

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QTextEdit>
#include <QToolBar>

namespace Visus {

//predeclaration
class FieldNode;
class TimeNode;
class IsoContourNode;
class IsocontourRenderNode;
class GLOrthoCamera;
class GLCameraNode;
class DatasetNode;
class QueryNode;
class ScriptingNode;
class PaletteNode;
class RenderArrayNode;
class ModelViewNode;
class KdRenderArrayNode;
class KdQueryNode;
class StatisticsNode;

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API ViewerPreferences
{
public:

  VISUS_CLASS(ViewerPreferences)

  String       title = "VisusViewer-" + OpenVisus_VERSION + "-" + OpenVisus_GIT_REVISION;

  bool         bShowTitleBar = true;
  bool         bShowToolbar = true;
  bool         bShowTreeView = true;
  bool         bShowDataflow = true;
  bool         bShowLogs = true;
  bool         bShowLogos = true;

  //in case you want to call Qt::setGeometry
  Rectangle2d  screen_bounds;

  //constructor
  ViewerPreferences() {
  }

  //write
  void write(Archive& ar) const
  {
    ar.write("title", title);
    ar.write("bShowTitleBar", bShowTitleBar);
    ar.write("bShowToolbar", bShowToolbar);
    ar.write("bShowTreeView", bShowTreeView);
    ar.write("bShowDataflow", bShowDataflow);
    ar.write("bShowLogs", bShowLogs);
    ar.write("bShowLogos", bShowLogos);
    ar.write("screen_bounds", screen_bounds);
  }

  //read
  void read(Archive& ar)
  {
    ar.read("title", title);
    ar.read("bShowTitleBar", bShowTitleBar);
    ar.read("bShowToolbar", bShowToolbar);
    ar.read("bShowTreeView", bShowTreeView);
    ar.read("bShowDataflow", bShowDataflow);
    ar.read("bShowLogs", bShowLogs);
    ar.read("bShowLogos", bShowLogos);
    ar.read("screen_bounds", screen_bounds);
  }

};

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API ViewerAutoRefresh
{
public:
  bool enabled = false;
  int  msec = 0;
};

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API ViewerToolBarTab : public QHBoxLayout
{
public:

  String name;

  //constructor
  ViewerToolBarTab(String name_) : name(name_) {
  }

  //constructor
  virtual ~ViewerToolBarTab() {
  }

  //createButton
  static QToolButton* createButton(QIcon icon, String name, std::function<void(bool)> clicked = std::function<void(bool)>())
  {
    auto ret = new QToolButton();

    //ret->setIconSize(QSize(24, 24));
    //ret->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

#if WIN32
    ret->setStyleSheet(
      "QToolButton       {background: transparent;} "
      "QToolButton:hover {background: lightGray ; border: 1px solid #6593cf;} ");
#endif

    if (!icon.isNull())
      ret->setIcon(icon);

    if (!name.empty())
      ret->setText(name.c_str());

    if (clicked)
      QObject::connect(ret, &QToolButton::clicked, clicked);

    return ret;
  }

  //createButton
  static QWidget* createButton(QAction* action)
  {
    auto ret = createButton(action->icon(), cstring(action->text()), [action](bool) {
      action->trigger();
    });

    ret->setEnabled(action->isEnabled());

    connect(action, &QAction::changed, [ret, action]() {
      ret->setEnabled(action->isEnabled());
      ret->setText(action->text());
    });

    ret->setToolTip(action->toolTip());
    return ret;
  }

  //addAction
  void addAction(QAction* action) {
    addWidget(createButton(action));
  }

  //addMenu
  QToolButton* addMenu(QIcon icon, String name, QMenu* menu)
  {
    auto button = createButton(icon, name + " ");
    button->setMenu(menu);
    button->setPopupMode(QToolButton::InstantPopup);
    addWidget(button);
    return button;
  }

  //addBlueMenu
  QToolButton* addBlueMenu(QIcon icon, String name, QMenu* menu)
  {
    menu->setStyleSheet("QMenu { "
      //"font-size:18px; "
      "color:white;"
      "background-color: rgb(43,87,184);"
      "selection-background-color: rgb(43,87,140);}");

    return addMenu(icon, name, menu);
  }

};

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API ViewerToolBar : public QToolBar
{
public:

  QMenu* file_menu = nullptr;
  struct
  {
    QCheckBox* check = nullptr;
    QLineEdit* msec = nullptr;
  }
  auto_refresh;

  QToolButton* bookmarks_button = nullptr;

  QTabWidget* tabs = nullptr;

  //constructor
  ViewerToolBar() 
  {
    QToolBar::addWidget(tabs = new QTabWidget());
    QPalette p(this->palette());
    p.setColor(QPalette::Base, Qt::darkGray);
    this->setPalette(p);
  }

  //addTab
  void addTab(QLayout* tab, String name) {
    auto frame = new QFrame();
    frame->setLayout(tab);
    tabs->addTab(frame, name.c_str());
  }

};

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API ViewerLogo
{
public:
  String   filename;
  Point2d  pos;
  double   opacity = 0.5;
  Point2d  border;
  SharedPtr<GLTexture> tex;
};


////////////////////////////////////////////////////////////////////
class VISUS_GUI_API Viewer :
  public QMainWindow,
  public Dataflow::Listener,
  public Model
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(Viewer)

  //getInstance
  static Viewer* getInstance();

  //constructor
  Viewer(String title = "Visus Viewer");

  //destructor
  virtual ~Viewer();

  //getTypeName
  virtual String getTypeName() const override {
    return "Viewer";
  }

  //setWindowTitle
  void setWindowTitle(String value) {
    QMainWindow::setWindowTitle(value.c_str());
  }

  //showMaximized 
  void showMaximized();

  //configureFromArgs
  void configureFromArgs(std::vector<String> args);

  //this is needed for swig
  void* c_ptr() {
    return this;
  }

  //isRunning
  bool isRunning() const {
    return running.value;
  }

  //printInfo
  void printInfo(String msg);

  //showLicences
  void showLicences();

  //getModel
  Dataflow* getDataflow() {
    return dataflow.get();
  }

  //getGLCanvas
  GLCanvas* getGLCanvas() {
    return widgets.glcanvas;
  }

  //getGLCamera
  SharedPtr<GLCamera> getGLCamera() const {
    return this->glcamera;
  }

  //getDataset
  SharedPtr<Dataset> getDataset() const;

  //getTreeView
  DataflowTreeView* getTreeView() const {
    return widgets.treeview;
  }

  //getFrameView
  DataflowFrameView* getFrameView() const {
    return widgets.frameview;
  }

  //getLog
  QTextEdit* getLog() const {
    return widgets.log;
  }

  //getCurrentTime
  double getCurrentTime();

  //addDockWidget
  void addDockWidget(String name, QWidget* widget);

  //addDockWidget
  void addDockWidget(Qt::DockWidgetArea area, QDockWidget* dockwidget) {
    QMainWindow::addDockWidget(area, dockwidget);
  }

  //showNodeContextMenu
  virtual bool showNodeContextMenu(Node* node) {
    return false;
  }

  //showWidget
  void showWidget(QWidget* widget, String title="");

  //setBackgroundColor
  void setBackgroundColor(Color value) {
    this->background_color = value;
    postRedisplay();
  }

  //openFile
  bool openFile(String filename, Node* parent = nullptr);

  //openUrl
  bool openUrl(String url, Node* parent = nullptr);

  //saveFile
  void saveFile(String filename, bool bSaveHistory = false);

  //postRedisplay
  void postRedisplay();

  //playFile
  bool playFile(String filename);

  //takeSnapshot
  bool takeSnapshot(bool bOnlyCanvas = false, String filename = "");

  //editNode
  virtual void editNode(Node* node = nullptr);

  //beginFreeTransformOnQueryNode
  void beginFreeTransformOnQueryNode(QueryNode* node);

  //beginFreeTransformOnDatasetNode
  void beginFreeTransformOnDatasetNode(DatasetNode* node);

  //beginFreeTransformOnModelviewNode
  void beginFreeTransformOnModelviewNode(ModelViewNode* node);

  //endFreeTransform
  void endFreeTransform();

  //refreshActions
  void refreshActions();

  //idle
  void idle();

  //modelChanged
  virtual void modelChanged() override {
    postRedisplay();
  }

  //enableSaveSession
  void enableSaveSession();

  //dataflowBeforeProcessInput
  virtual void dataflowBeforeProcessInput(Node* node) override;

  //dataflowAfterProcessInput
  virtual void dataflowAfterProcessInput(Node* node) override;

public:

  //getRoot
  Node* getRoot() const {
    return dataflow->getRoot();
  }

  //getUUID
  String getUUID(Node* node) const {
    return node ? node->getUUID() : "";
  }

  //getNodes
  const ScopedVector<Node>& getNodes() const {
    return dataflow->getNodes();
  }

  //findNodeByUUID
  Node* findNodeByUUID(const String& uuid) const {
    return dataflow->findNodeByUUID(uuid);
  }

  //findNodeByType
  template <class ClassName>
  ClassName* findNode() const
  {
    for (auto node : getNodes()) {
      if (const ClassName* ret = dynamic_cast<const ClassName*>(node))
        return const_cast<ClassName*>(ret);
    }
    return nullptr;
  }

  //findPick
  Node* findPick(Node* node, Point2d screen_point, bool bRecursive, double* distance = nullptr) const;

  //computeNodeBounds
  Position computeNodeBounds(Node* node) const;

  //getWorldDimension
  int getWorldDimension() const;

  //getWorldBox
  BoxNd getWorldBox() const;

  //computeQueryBoundsInDatasetCoordinates
  Position computeQueryBoundsInDatasetCoordinates(QueryNode* query_node) const;

  //computeNodeToScreen
  Frustum computeNodeToScreen(Frustum frustum, Node* node) const;

public:

  //attachGLCamera
  virtual void attachGLCamera(SharedPtr<GLCamera> value);

  //detachGLCamera
  virtual void detachGLCamera();

  //glGetRenderQueue
  virtual int glGetRenderQueue(Node* node);

  //glCameraChangeEvent
  virtual void glCameraChangeEvent();

public:

  //glCanvasResizeEvent
  virtual void glCanvasResizeEvent(QResizeEvent* evt);

  //glCanvasMousePressEvent
  virtual void glCanvasMousePressEvent(QMouseEvent* evt);

  //glCanvasMouseMoveEvent
  virtual void glCanvasMouseMoveEvent(QMouseEvent* evt);

  //glCanvasMouseReleaseEvent
  virtual void glCanvasMouseReleaseEvent(QMouseEvent* evt);

  //glCanvasWheelEvent
  virtual void glCanvasWheelEvent(QWheelEvent* evt);
  
public:

  //keyPressEvent
  virtual void keyPressEvent(QKeyEvent* evt) override;

public:

  //glRender
  virtual void glRender(GLCanvas& gl);

  //glRenderNodes
  virtual void glRenderNodes(GLCanvas& gl);

  //glRenderSelection
  virtual void glRenderSelection(GLCanvas& gl);

  //glRenderGestures
  virtual void glRenderGestures(GLCanvas& gl);

  //glRenderLogos
  virtual void glRenderLogos(GLCanvas& gl);

public:

  //clearAll
  void clearAll();

  //dropProcessing
  void dropProcessing();

  //getAutoRefresh
  const ViewerAutoRefresh& getAutoRefresh() const {
    return this->auto_refresh;
  }

  //setAutoRefresh
  void setAutoRefresh(ViewerAutoRefresh value);

  //getSelection
  Node* getSelection() const {
    return dataflow->getSelection();
  }

  //setSelection
  void setSelection(Node* node);

  //dropSelection
  void dropSelection() {
    setSelection(nullptr);
  }

  //setMinimal
  void setMinimal();

  //setNodeName
  void setNodeName(Node* node, String value);

  //setNodeVisible
  void setNodeVisible(Node* node, bool value);

  //addNode
  void addNode(Node* parent, Node* VISUS_DISOWN(node), int index = -1);

  //addNode
  void addNode(Node* VISUS_DISOWN(node)) {
    addNode(nullptr, node);
  }

  //removeNode
  void removeNode(Node* node);

  void removeNode(String uuid) {
    if (Node* node = findNodeByUUID(uuid))
      removeNode(node);
  }

  //moveNode
  void moveNode(Node* dst, Node* src, int index = -1);

  //connectNodes
  void connectNodes(Node* from, String oport_name, String iport_name, Node* to);

  //connectNodes
  void connectNodes(Node* from, String port, Node* to);

  //connectNodes
  void connectNodes(Node* from, Node* to);

  //disconnectNodes
  void disconnectNodes(Node* from, String oport_name, String iport_name, Node* to);

  //autoConnectNodes
  void autoConnectNodes();

  //isMouseDragging
  bool isMouseDragging() const {
    return mouse_dragging;
  }

  //setMouseDragging
  void setMouseDragging(bool value);

  //scheduleMouseDragging
  void scheduleMouseDragging(bool value, int msec);

  //reloadVisusConfig
  void reloadVisusConfig(bool bChooseAFile = false);

  //setPreferences
  void setPreferences(ViewerPreferences value);

  //open
  bool open(String url, Node* parent = nullptr);

  //save
  void save(String filename, bool bSaveHistory = false);

  //openSnapshot
  bool openSnapshot(bool prev=false);

  //saveSnapshot
  bool saveSnapshot();

  //enableHistory
  void enableHistory();

  //setDataflow
  void setDataflow(SharedPtr<Dataflow> dataflow);

  //refreshNode
  void refreshNode(Node* node = nullptr);

  //refreshAll
  void refreshAll() {
    refreshNode(nullptr);
  }

  //guessGLCameraPosition
  void guessGLCameraPosition(int ref_ = -1);

  //mirrorGLCamera
  void mirrorGLCamera(int ref = 0);

  //addWorld
  Node* addWorld(String uuid);

  //addDataset
  DatasetNode* addDataset(String uuid, Node* parent, String url);

  //loadDataset
  SharedPtr<Dataset> loadDataset(String url);

  //addGLCameraNode
  GLCameraNode* addGLCamera(String uuid, Node* parent, String type = "");

  //addVolume
  QueryNode* addVolume(String uuid, Node* parent, String fieldname = "", int access_id = 0);

  //addSlice
  QueryNode* addSlice(String uuid, Node* parent, String fieldname = "", int access_id = 0, int axis=2);

  //setSlicePosition (sperimental for slices)
  void setSlicePosition(QueryNode* query_node, int axis, double offset, Position target_pos = Position::invalid());

  //setFieldName
  void setFieldName(String value);

  //addKdQuery
  KdQueryNode* addKdQuery(String uuid, Node* parent, String fieldname = "", int access_id = 0);

  //addIsoContour
  QueryNode* addIsoContour(String uuid, Node* parent, String fieldname = "", int access_id = 0, String isovalue = "");

  //addScripting
  ScriptingNode* addScripting(String uuid, Node* parent);

  //setScriptingCode
  void setScriptingCode(String value);

  //addStatistics
  StatisticsNode* addStatistics(String uuid, Node* parent);

  //addRender
  Node* addRender(String uuid, Node* parent, String palette = "");

  //addKdRender
  KdRenderArrayNode* addKdRender(String uuid, Node* parent, String palette = "");

  //addGroup
  Node* addGroup(String uuid, Node* parent, String name = "");

  //addModelView
  ModelViewNode* addModelView(String uuid, Node* parent, bool insert = false);

  //addPalette
  PaletteNode* addPalette(String uuid, Node* parent, String palette);

public:

  //rotateCamera
  void  rotateCamera(Point3d axis, double angle = 5);;

public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

public:

  //addNetRcv
  bool addNetRcv(int port);

  //addNetSnd
  bool addNetSnd(String out_url, Rectangle2d split_ortho = Rectangle2d(0, 0, 1, 1), Rectangle2d screen_bounds = Rectangle2d(), double fix_aspect_ratio = 0);

private:

  //________________________________________________________
  class Icons
  {
  public:

    QIcon world = QIcon(":/world.png");
    QIcon camera = QIcon(":/camera.png");
    QIcon clock = QIcon(":/clock.png");
    QIcon cpu = QIcon(":/cpu.png");
    QIcon dataset = QIcon(":/database.png");
    QIcon gear = QIcon(":/gear.png");
    QIcon paint = QIcon(":/paint.png");
    QIcon statistics = QIcon(":/statistics.png");
    QIcon document = QIcon(":/document.png");
    QIcon group = QIcon(":/group.png");
    QIcon palette = QIcon(":/palette.png");
    QIcon brush = QIcon(":/brush.png");

    //constructor
    Icons() {
    }

  };
  //________________________________________________________
  class Running
  {
  public:
    Time   t1;
    bool   value = false;
    double elapsed = 0;


    Time   t2;
    Int64  io_rbytes = 0, io_rbytes_persec = 0;
    Int64  io_wbytes = 0, io_wbytes_persec = 0;

  };

  //________________________________________________
  class GuiActions
  {
  public:

    QAction * New = nullptr;
    QAction* OpenFile = nullptr;
    QAction* AddFile = nullptr;
    QAction* SaveFile = nullptr;
    QAction* SaveFileAs = nullptr;
    QAction* SaveSceneAs = nullptr;
    QAction* SaveHistoryAs = nullptr;
    QAction* SaveSnapshot = nullptr;
    QAction* OpenNextSnapshot = nullptr;
    QAction* OpenPreviousSnapshot = nullptr;
    QAction* OpenUrl = nullptr;
    QAction* AddUrl = nullptr;
    QAction* ReloadVisusConfig = nullptr;
    QAction* PlayFile = nullptr;
    QAction* Close = nullptr;

    QAction* RefreshNode = nullptr;
    QAction* DropProcessing = nullptr;
    QAction* WindowSnapShot = nullptr;
    QAction* CanvasSnapShot = nullptr;
    QAction* MirrorX = nullptr;
    QAction* MirrorY = nullptr;
    QAction* FitBest = nullptr;
    QAction* CameraX = nullptr;
    QAction* CameraY = nullptr;
    QAction* CameraZ = nullptr;

    QAction* RotatePosX = nullptr;
    QAction* RotatePosY = nullptr;
    QAction* RotatePosZ = nullptr;
    QAction* RotateNegX = nullptr;
    QAction* RotateNegY = nullptr;
    QAction* RotateNegZ = nullptr;

    QAction* EditNode = nullptr;
    QAction* RemoveNode = nullptr;
    QAction* RenameNode = nullptr;
    QAction* ShowHideNode = nullptr;
    QAction* Undo = nullptr;
    QAction* Redo = nullptr;
    QAction* Deselect = nullptr;

    QAction* AddGroup = nullptr;
    QAction* AddTransform = nullptr;
    QAction* InsertTransform = nullptr;
    QAction* AddSliceX = nullptr;
    QAction* AddSliceY = nullptr;
    QAction* AddSliceZ = nullptr;
    QAction* AddVolume = nullptr;
    QAction* AddIsoContour = nullptr;
    QAction* AddKdQuery = nullptr;
    QAction* AddRender = nullptr;
    QAction* AddKdRender = nullptr;
    QAction* AddScripting = nullptr;
    QAction* AddStatistics = nullptr;

    QAction* ShowLicences = nullptr;

  };

  //________________________________________________
  class Widgets
  {
  public:

    //permantent
    ViewerToolBar *    toolbar = nullptr;
    QTextEdit*         log = nullptr;

    //non permanent
    QTabWidget*        tabs = nullptr;
    DataflowTreeView*  treeview = nullptr;
    DataflowFrameView* frameview = nullptr;
    GLCanvas*          glcanvas = nullptr;
  };

  //________________________________________________
  class NetConnection
  {
  public:

    SharedPtr<NetSocket>             socket = std::make_shared<NetSocket>();
    bool                             bSend = true;
    String                           url;
    std::ofstream                    log;

    QTimer                           timer;

    CriticalSection                  requests_lock;
    std::vector<NetRequest>          requests;
    int                              request_id = 0;

    Rectangle2d                      split_ortho = Rectangle2d(0, 0, 1, 1);
    double                           fix_aspect_ratio = 0;

    SharedPtr<std::thread>           thread;
    bool                             bExitThread = false;

    //destructor
    virtual ~NetConnection()
    {
      bExitThread = true;
      if (thread && thread->joinable()) {
        this->socket->close();
        Thread::join(thread);
      }
    }

  };

  typedef std::vector< SharedPtr<NetConnection> > Connections;

  SharedPtr<Dataflow>                   dataflow;

  //run time (i.e. don't need to be saved)
  UniquePtr<QTimer>                     idle_timer;
  UniquePtr<QTimer>                     save_session_timer;
  bool                                  mouse_dragging = false;
  SharedPtr<FreeTransform>              free_transform;
  SharedPtr<Icons>                      icons;
  GuiActions                            actions;
  Widgets                               widgets;
  Running                               running;
  String                                last_saved_filename;
  std::vector< SharedPtr<ViewerLogo> >  logos;
  ViewerPreferences                     preferences;
  Connections                           netrcv;
  Connections                           netsnd;
  Color                                 background_color;
  ViewerAutoRefresh                     auto_refresh;
  SharedPtr<QTimer>                     auto_refresh_timer;
  ConfigFile                            config;

  GLMouse                               mouse;
  UniquePtr<QTimer>                     mouse_timer;

  SharedPtr<GLCamera>                   glcamera;
  Slot<void()>                          glcamera_end_update_slot;
  Slot<void()>                          glcamera_redisplay_needed_slot;

  String                                last_filename = "";


  std::vector< SharedPtr<GLObject> >    huds;
  SharedPtr<NetServer>                  server;

  struct
  {
    CriticalSection                     lock;
    std::vector<String>                 messages;
    std::ofstream                       fstream;
  }
  log;

  struct
  {
    SharedPtr<QTimer> timer;
    std::deque<StringTree> actions;
  }
  scheduled;

  struct
  {
    String dir = ".";
    String current;
    std::vector<String> list;
  }
  snapshots;

  //openScreenLogo
  SharedPtr<ViewerLogo> openScreenLogo(String key, String default_logo);

  //internalFlushMessages
  void internalFlushMessages();

  //sendNetMessage
  void sendNetMessage(SharedPtr<NetConnection> netsnd,void* obj);

  //createActions
  void createActions();

  //createBookmarks
  void createBookmarks(QMenu* dst, const StringTree& src);

  //createBookmarks
  QMenu* createBookmarks();

  //createTreeView
  DataflowTreeView* createTreeView();

  //createGLCanvas
  GLCanvas* createGLCanvas();

  //createToolBar
  void createToolBar();

signals:

  void postFlushMessages();

}; //end class


} //namespace Visus

#endif //VISUS_VIEWER_H

