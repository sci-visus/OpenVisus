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

#include <Visus/AppKit.h>
#include <Visus/Model.h>
#include <Visus/FreeTransform.h>
#include <Visus/GLCamera.h>
#include <Visus/Dataset.h>
#include <Visus/GLCameraNode.h>
#include <Visus/DataflowFrameView.h>
#include <Visus/DataflowTreeView.h>
#include <Visus/Thread.h>
#include <Visus/NetSocket.h>
#include <Visus/ApplicationInfo.h>
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
class OSPRayRenderNode;
class ModelViewNode;
class KdRenderArrayNode;
class KdQueryNode;
class CpuPaletteNode;
class StatisticsNode;

//predeclaration
class Viewer;

////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API ViewerPlugin
{
public:

  VISUS_NON_COPYABLE_CLASS(ViewerPlugin)

  Viewer* viewer;

  //constructor
  ViewerPlugin(Viewer* viewer_ = nullptr) : viewer(viewer_) {
  }

  //destructor
  virtual ~ViewerPlugin() {
  }

  //getPluginName
  virtual String getPluginName() = 0;

};


////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API Viewer :
  public QMainWindow,
  public Dataflow::Listener,
  public Model
{
  Q_OBJECT

public:

  //_____________________________________________________________
  class VISUS_APPKIT_API AutoRefresh
  {
  public:
    bool enabled = false;
    int  msec = 0;
  };

  //_____________________________________________________________
  class VISUS_APPKIT_API Defaults
  {
  public:
    static String panels;
    static bool   show_logos;
  };

  //_____________________________________________________________
  class VISUS_APPKIT_API Preferences
  {
  public:

    VISUS_CLASS(Preferences)

    String       title = "VisusViewer-" + ApplicationInfo::git_revision;
    String       panels;
    bool         bHideTitleBar = false;
    bool         bHideMenus = false;
    bool         bRightHanded = true;
    Rectangle2d  screen_bounds;
    bool         show_logos=true;

    //constructor
    Preferences() {
      this->panels = Defaults::panels;
      this->show_logos = Defaults::show_logos;
    }

    //writeTo
    void writeTo(StringTree& out) const
    {
      out.writeValue("title", title);
      out.writeValue("panels", panels);
      out.writeValue("bHideTitleBar", cstring(bHideTitleBar));
      out.writeValue("bHideMenus", cstring(bHideMenus));
      out.writeValue("screen_bounds", screen_bounds.toString());
    }

    //readFrom
    void readFrom(StringTree& in)
    {
      title = in.readValue("title");
      panels = in.readValue("panels");
      bHideTitleBar = cbool(in.readValue("bHideTitleBar"));
      bHideMenus = cbool(in.readValue("bHideMenus"));
      screen_bounds = Rectangle2d(in.readValue("screen_bounds"));
    }

  };

  typedef ViewerPlugin Plugin;

  typedef std::vector< SharedPtr<Plugin> > Plugins;

  VISUS_NON_COPYABLE_CLASS(Viewer)

  //constructor
  Viewer(String title = "Visus Viewer");

  //destructor
  virtual ~Viewer();


  //getTypeName
  virtual String getTypeName() const override {
    return "Viewer";
  }

  //getUUID
  String getUUID(Node* node) const {
    return node ? node->getUUID() : "";
  }

  //this is needed for swig
  void* c_ptr() {
    return this;
  }

  //setMinimal
  void setMinimal();

  //setFieldName
  void setFieldName(String value);

  //setScriptingCode
  void setScriptingCode(String value);

  //configureFromCommandLine
  void configureFromCommandLine(std::vector<String> args);

  //showLicences
  void showLicences();

  //executeAction
  virtual void executeAction(StringTree in) override;

  //getModel
  Dataflow* getDataflow() {
    return dataflow.get();
  }

  //getNodes
  const ScopedVector<Node>& getNodes() const {
    return dataflow->getNodes();
  }

  //findNodeByUUID
  Node* findNodeByUUID(const String& uuid) const {
    return dataflow->findNodeByUUID(uuid);
  }

  //findNodeByName
  Node* findNodeByName(const String& name) const {
    return dataflow->findNodeByName(name);
  }

  //findNodeByType
  template <class ClassName>
  ClassName* findNodeByType() const
  {
    for (auto node : getNodes()) {
      if (const ClassName* ret = dynamic_cast<const ClassName*>(node))
        return const_cast<ClassName*>(ret);
    }
    return nullptr;
  }

  //dropProcessing
  void dropProcessing();

  //getRoot
  Node* getRoot() const {
    return dataflow->getRoot();
  }

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

  //getAutoRefresh
  const AutoRefresh& getAutoRefresh() const {
    return this->auto_refresh;
  }

  //setAutoRefresh
  void setAutoRefresh(AutoRefresh value);

  //beginFreeTransform
  void beginFreeTransform(QueryNode* node);

  //beginFreeTransform
  void beginFreeTransform(ModelViewNode* node);

  //endFreeTransform
  void endFreeTransform();

  //computeNodeToNode
  Position computeNodeToNode(Node* dst, Node* src) const;

  //computeQueryBounds
  Position computeQueryBounds(QueryNode* query_node) const;

  //computeNodeToScreen
  Frustum computeNodeToScreen(Frustum frustum, Node* node) const;

  //New
  void New();

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

  //moveNode
  void moveNode(Node* dst, Node* src, int index = -1);

  //connectPorts
  void connectPorts(Node* from, String oport_name, String iport_name, Node* to);

  //connectPorts
  void connectPorts(Node* from, String port_name, Node* to) {
    connectPorts(from, port_name, port_name, to);
  }

  //connectPorts
  void connectPorts(Node* from, Node* to)
  {
    if (from->outputs.size() == 1) { connectPorts(from, from->getFirstOutputPort()->getName(), to); return; }
    if (to->inputs.size() == 1) { connectPorts(from, to->getFirstInputPort()->getName(), to); return; }
    VisusAssert(false);
  }
  //disconnectPorts
  void disconnectPorts(Node* from, String oport_name, String iport_name, Node* to);

  //autoConnectPorts
  void autoConnectPorts();

  //fastRendering
  bool fastRendering() const {
    return fast_rendering;
  }

  //writeConfigString
  void setFastRendering(bool value);

  //reloadVisusConfig
  void reloadVisusConfig(bool bChooseAFile = false);

  //setPreferences
  void setPreferences(Preferences value);

  //open
  bool open(String url, Node* parent = nullptr, bool bShowUrlDialogIfNeeded = false);

  //save
  bool save(String filename, bool bSaveHistory = false, bool bShowDialogs = true);

  //takeSnapshot
  bool takeSnapshot(bool bOnlyCanvas = false, String filename = "");

  //editNode
  void editNode(Node* node = nullptr);

  //setDataflow
  void setDataflow(SharedPtr<Dataflow> dataflow);

  //getGLCamera
  SharedPtr<GLCamera> getGLCamera() const {
    return widgets.glcanvas ? widgets.glcanvas->glcamera : nullptr;
  }

  //refreshData
  void refreshData(Node* node = nullptr);

  //guessGLCameraPosition
  void guessGLCameraPosition(int ref_ = -1);

  //mirrorGLCamera
  void mirrorGLCamera(int ref = 0);

  //getWorldDimension
  int getWorldDimension() const;

  //getWorldBox
  BoxNd getWorldBox() const;

  //getPosition
  Position getPosition(Node* node, bool bRecursive = false) const;

  //getGLCanvas
  GLCanvas* getGLCanvas() {
    return widgets.glcanvas;
  }

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

  //showNodeContextMenu
  virtual bool showNodeContextMenu(Node *node) {
    return false;
  }

  //postRedisplay
  void postRedisplay();

  //addDockWidget
  void addDockWidget(String name, QWidget* widget);

  //addDockWidget
  void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget) {
    QMainWindow::addDockWidget(area, dockwidget);
  }

  //showPopupWidget
  void showPopupWidget(QWidget* widget);

public:

  //addDataset
  DatasetNode* addDataset(Node* parent, SharedPtr<Dataset> dataset);

  //addDataset
  DatasetNode* addDataset(Node* parent, String url,StringTree config = StringTree()) {
    return addDataset(parent, LoadDatasetEx(url, config ? config : this->config));
  }

  //addGLCameraNode
  GLCameraNode* addGLCamera(Node* parent, SharedPtr<GLCamera> glcamera);

  //addGLCameraNode
  GLCameraNode* addGLCamera(Node* parent, String type);

  //addVolume
  QueryNode* addVolume(Node* parent, String fieldname="", int access_id=0);

  //addSlice
  QueryNode* addSlice(Node* parent, String fieldname = "", int access_id = 0);

  //addIsoContour
  QueryNode* addIsoContour(Node* parent, String fieldname = "", int access_id = 0);

  //addKdQuery
  KdQueryNode* addKdQuery(Node* parent, String fieldname = "", int access_id = 0);

  //addScripting
  ScriptingNode* addScripting(Node* parent);

  //addCpuTransferFunction
  CpuPaletteNode* addCpuTransferFunction(Node* parent);

  //addStatistics
  StatisticsNode* addStatistics(Node* parent);

  //addRender
  Node* addRender(Node* parent, String palette = "");

  //addKdRender
  KdRenderArrayNode* addKdRender(Node* parent);

  //addOSPRay
  Node* addOSPRay(Node* parent, String palette = "");

  //addGroup
  Node* addGroup(Node* parent, String name = "");

  //addModelView
  ModelViewNode* addModelView(Node* parent, bool insert=false);

  //addPalette
  PaletteNode* addPalette(Node* parent, String palette);

public:

  //writeTo
  virtual void writeTo(StringTree& out) const override;

  //readFrom
  virtual void readFrom(StringTree& out) override;

public:

  //addNetRcv
  bool addNetRcv(int port);

  //addNetSnd
  bool addNetSnd(String out_url, Rectangle2d split_ortho = Rectangle2d(0, 0, 1, 1), Rectangle2d screen_bounds = Rectangle2d(), double fix_aspect_ratio = 0);

  //getPlugins
  Plugins& getPlugins() {
    return plugins;
  }

private:

  //setBlueMenu
  static void setBlueMenu(QMenu* menu)
  {
    menu->setStyleSheet("QMenu { "
      //"font-size:18px; "
      "color:white;"
      "background-color: rgb(43,87,184);"
      "selection-background-color: rgb(43,87,140);}");
  }

  //________________________________________________________
  class ToolBar : public QToolBar
  {
  public:

    QMenu * file_menu = nullptr;
    struct
    {
      QCheckBox*   check = nullptr;
      QLineEdit*   msec = nullptr;
    }
    auto_refresh;
    QToolButton* bookmarks_button = nullptr;

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

    //___________________________________________
    class TabLayout : public QHBoxLayout
    {
    public:

      String name;

      //constructor
      TabLayout(String name_) : name(name_) {
      }

      //constructor
      virtual ~TabLayout() {
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
        setBlueMenu(menu);
        return addMenu(icon, name, menu);
      }

    };

    QTabWidget*  tabs = nullptr;

    //constructor
    ToolBar() {
      QToolBar::addWidget(tabs = new QTabWidget());

      QPalette p(this->palette());
      p.setColor(QPalette::Base, Qt::darkGray);
      this->setPalette(p);
    }

    //addTab
    void addTab(TabLayout* tab) {
      auto frame = new QFrame();
      frame->setLayout(tab);
      tabs->addTab(frame, tab->name.c_str());
    }

    //addTab
    VISUS_NEWOBJECT(TabLayout*) addTab(String name)
    {
      auto tab = new TabLayout(name);
      addTab(tab);
      return tab;
    }

  };

  //________________________________________________________
  class Logo
  {
  public:
    String   filename;
    Point2d  pos;
    double   opacity = 0.5;
    Point2d  border;
    SharedPtr<GLTexture> tex;
  };

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
  class MyGLCanvas : public GLCanvas
  {
  public:

    GLMouse             mouse;
    UniquePtr<QTimer>   mouse_timer;

    SharedPtr<GLCamera> glcamera = nullptr;
    Slot<void()>        glcamera_end_update_slot;
    Slot<void()>        glcamera_redisplay_needed_slot;
  };

  //________________________________________________________
  class Running
  {
  public:
    bool   value = false;
    Time   t1;
    double enlapsed = 0;
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
    QAction* OpenUrl = nullptr;
    QAction* AddUrl = nullptr;
    QAction* ReloadVisusConfig = nullptr;
    QAction* Close = nullptr;

    QAction* RefreshData = nullptr;
    QAction* DropProcessing = nullptr;
    QAction* WindowSnapShot = nullptr;
    QAction* CanvasSnapShot = nullptr;
    QAction* MirrorX = nullptr;
    QAction* MirrorY = nullptr;
    QAction* FitBest = nullptr;
    QAction* CameraX = nullptr;
    QAction* CameraY = nullptr;
    QAction* CameraZ = nullptr;

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
    QAction* AddSlice = nullptr;
    QAction* AddVolume = nullptr;
    QAction* AddIsoContour = nullptr;
    QAction* AddKdQuery = nullptr;
    QAction* AddRender = nullptr;
    QAction* AddKdRender = nullptr;
    QAction* AddScripting = nullptr;
    QAction* AddStatistics = nullptr;
    QAction* AddCpuTransferFunction = nullptr;

    QAction* ShowLicences = nullptr;

  };

  //________________________________________________
  class Widgets
  {
  public:

    //permantent
    ToolBar * toolbar = nullptr;
    QTextEdit*                           log = nullptr;

    //non permanent
    QTabWidget*                          tabs = nullptr;
    DataflowTreeView*                    treeview = nullptr;
    DataflowFrameView*                   frameview = nullptr;
    MyGLCanvas*                          glcanvas = nullptr;
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
  bool                                  fast_rendering = false;
  SharedPtr<FreeTransform>              free_transform;
  SharedPtr<Icons>                      icons;
  GuiActions                            actions;
  Widgets                               widgets;
  Running                               running;
  Plugins                               plugins;
  String                                last_saved_filename;
  std::vector< SharedPtr<Logo> >        logos;
  Preferences                           preferences;
  Connections                           netrcv;
  Connections                           netsnd;
  Color                                 background_color;
  AutoRefresh                           auto_refresh;
  SharedPtr<QTimer>                     auto_refresh_timer;
  ConfigFile                            config;

  struct
  {
    CriticalSection                       lock;
    std::vector<String>                   messages;
    std::ofstream                         fstream;
  }
  log;

  SharedPtr<Logo> openScreenLogo(String key, String default_logo);

  //internalFlushMessages
  void internalFlushMessages();

  //sendNetMessage
  void sendNetMessage(SharedPtr<NetConnection> netsnd,void* obj);

  //createActions
  void createActions();

  //refreshActions
  void refreshActions();

  //createBookmarks
  void createBookmarks(QMenu* dst,const StringTree& src);

  //createBookmarks
  QMenu* createBookmarks() {
    auto ret=new QMenu(this);
    createBookmarks(ret,this->config);
    setBlueMenu(ret);
    return ret;
  }

  //idle
  void idle();

  //modelChanged
  virtual void modelChanged() override {
    postRedisplay();
  }

  //findPick
  Node* findPick(Node* node,Point2d screen_point,bool bRecursive,double* distance=nullptr) const;

  //attachGLCamera
  void attachGLCamera(SharedPtr<GLCamera> value);

  //detachGLCamera
  void detachGLCamera();

  //glGetRenderQueue
  int glGetRenderQueue(Node* node);

  //glCameraChangeEvent
  void glCameraChangeEvent();

  //glCanvasResizeEvent
  void glCanvasResizeEvent(QResizeEvent* evt);

  //glCanvasMousePressEvent
  void glCanvasMousePressEvent(QMouseEvent* evt);

  //glCanvasMouseMoveEvent
  void glCanvasMouseMoveEvent(QMouseEvent* evt);

  //glCanvasMouseReleaseEvent
  void glCanvasMouseReleaseEvent(QMouseEvent* evt);

  //glCanvasWheelEvent
  void glCanvasWheelEvent(QWheelEvent* evt);

  //glRender
  void glRender(GLCanvas& gl);

  //keyPressEvent
  virtual void keyPressEvent(QKeyEvent* evt) override;

  //enableSaveSession
  void enableSaveSession();

  //createTreeView
  DataflowTreeView* createTreeView();

  //createGLCanvas
  MyGLCanvas* createGLCanvas();

  //createToolBar
  void createToolBar();

  //dataflowBeforeProcessInput
  virtual void dataflowBeforeProcessInput(Node* node) override;

  //dataflowAfterProcessInput
  virtual void dataflowAfterProcessInput(Node* node) override;

  //glRenderNodes
  void glRenderNodes(GLCanvas& gl);

  //glRenderSelection
  void glRenderSelection(GLCanvas& gl);

  //glRenderGestures
  void glRenderGestures(GLCanvas& gl);

  //glRenderLogos
  void glRenderLogos(GLCanvas& gl);

signals:

  void postFlushMessages();

private:

  std::vector< SharedPtr<GLObject> > huds;
  SharedPtr<NetServer> server;


}; //end class


} //namespace Visus

#endif //VISUS_VIEWER_H

