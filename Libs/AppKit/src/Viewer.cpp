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

#include <Visus/Path.h>
#include <Visus/RamResource.h>
#include <Visus/File.h>
#include <Visus/Thread.h>
#include <Visus/NetSocket.h>
#include <Visus/Diff.h>

#include <Visus/Dataflow.h>
#include <Visus/IdxDataset.h>

#include <Visus/GLCamera.h>
#include <Visus/GLInfo.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/GLObjects.h>

#include <Visus/FieldNode.h>
#include <Visus/TimeNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLCameraNode.h>
#include <Visus/DatasetNode.h>
#include <Visus/QueryNode.h>
#include <Visus/ScriptingNode.h>
#include <Visus/PaletteNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/OSPRayRenderNode.h>
#include <Visus/ModelViewNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/KdQueryNode.h>
#include <Visus/CpuPaletteNode.h>
#include <Visus/StatisticsNode.h>
#include <Visus/NetService.h>
#include <Visus/ModVisus.h>
#include <Visus/Rectangle.h>

#include <Visus/FieldNode.h>
#include <Visus/GLCameraNode.h>

#include <QDockWidget>
#include <QStatusBar>
#include <QApplication>
#include <QFontDatabase>
#include <QStyle>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>

namespace Visus {

String Viewer::Defaults::panels = "left center";
bool   Viewer::Defaults::show_logos = true;

///////////////////////////////////////////////////////////////////////////////////////////////
Viewer::Viewer(String title) : QMainWindow()
{
  this->config = *AppKitModule::getModuleConfig();

  RedirectLog=[this](const String& msg) {
    {
      ScopedLock lock(log.lock);
      log.messages.push_back(msg);
    };

    //I can be here in different thread
    //see //see http://stackoverflow.com/questions/37222069/start-qtimer-from-another-class
    emit postFlushMessages();
  };

  connect(this, &Viewer::postFlushMessages, this, &Viewer::internalFlushMessages, Qt::QueuedConnection);

  this->log.fstream.open(KnownPaths::VisusHome.getChild("visus." + Time::now().getFormattedLocalTime()+ ".log"));

  setWindowTitle(title.c_str());

  this->background_color=Color::fromString(config.readString("Configuration/VisusViewer/background_color", Colors::DarkBlue.toString()));

  //logos
  {
    if (auto logo = openScreenLogo("Configuration/VisusViewer/Logo/BottomLeft" , ":sci.png"  )) logos.push_back(logo);
    if (auto logo = openScreenLogo("Configuration/VisusViewer/Logo/BottomRight", ":visus.png")) logos.push_back(logo);
    if (auto logo = openScreenLogo("Configuration/VisusViewer/Logo/TopRight"   , ""          )) logos.push_back(logo);
    if (auto logo = openScreenLogo("Configuration/VisusViewer/Logo/TopLeft"    , ""          )) logos.push_back(logo);
  }

  icons.reset(new Icons());
  createActions();
  createToolBar();

  //status bar
  setStatusBar(new QStatusBar());

  //log
  {
    widgets.log=GuiFactory::CreateTextEdit(Colors::Black,Color(230,230,230));

    auto dock = new QDockWidget("Log");
    dock->setWidget(widgets.log);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
  }

#ifdef VISUS_DEBUG
  enableLog("visusviewer.log.txt");
#endif

  New();

  refreshActions();
  
  setFocusPolicy(Qt::StrongFocus);
  
  showMaximized();
}

////////////////////////////////////////////////////////////
Viewer::~Viewer()
{
  VisusInfo() << "destroying VisusViewer";
  RedirectLog = nullptr;
  setDataflow(nullptr);
}


////////////////////////////////////////////////////////////
void Viewer::executeAction(StringTree in)
{

  //action directed to nodes?
  if (getPassThroughAction(in, "nodes"))
  {
    auto uuid=popTargetId(in);
    auto node = findNodeByUUID(uuid); VisusAssert(node);
    node->executeAction(in);
    return;
  }

  if (in.name == "set")
  {
    auto target_id = in.readString("target_id");

    if (target_id == "mouse_dragging")
    {
      setMouseDragging(in.readBool("value"));
      return;
    }

    if (target_id == "auto_refresh")
    {
      AutoRefresh value = getAutoRefresh();
      value.enabled = in.readBool("enabled");
      value.msec = in.readInt("msec");
      setAutoRefresh(value);
      return;
    }
  }

  if (in.name == "open")
  {
    auto parent = findNodeByUUID(in.readString("parent"));
    auto url = in.readString("url");
    open(url,parent);
    return;
  }

  if (in.name == "moveNode")
  {
    auto src = findNodeByUUID(in.readString("dst"));
    auto dst = findNodeByUUID(in.readString("src"));
    auto index = in.readInt("index", -1);
    moveNode(dst, src, index);
    return;
  }

  if (in.name == "setSelection")
  {
    auto node = findNodeByUUID(in.readString("node"));
    setSelection(node);
    return;
  }

  if (in.name == "connectPorts")
  {
    auto from = findNodeByUUID(in.readString("from"));
    auto oport = in.readString("oport");
    auto iport = in.readString("iport");
    auto to = findNodeByUUID(in.readString("to"));
    connectPorts(from, oport, iport, to);
    return;
  }

  if (in.name == "disconnectPorts")
  {
    auto from = findNodeByUUID(in.readString("from"));
    auto oport = in.readString("oport");
    auto iport = in.readString("iport");
    auto to = findNodeByUUID(in.readString("to"));
    disconnectPorts(from, oport, iport, to);
    return;
  }

  if (in.name == "refreshData")
  {
    auto node = findNodeByUUID(in.readString("node"));
    refreshData(node);
    return;
  }

  if (in.name == "dropProcessing")
  {
    this->dropProcessing();
    return;
  }

  if (in.name == "showNode") {
    auto node = findNodeByUUID(in.readString("node"));
    auto value = in.readBool("value");
    setNodeVisible(node, value);
    return;
  }

  if (in.name=="add")
  {
    auto what = in.readString("what");

    if (what == "node")
    {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto child = *in.getFirstChild();
      auto TypeName = child.name;
      auto node = NodeFactory::getSingleton()->createInstance(TypeName);
      node->readFrom(child);
      auto index = in.readInt("index", -1);
      addNode(parent, node, index);
      return;
    }

    if (what == "volume") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto fieldname = in.readString("fieldname");
      auto access_id = in.readInt("access_id");
      addVolume(parent, fieldname, access_id);
      return;
    }

    if (what == "slice") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto fieldname = in.readString("fieldname");
      auto access_id = in.readInt("access_id");
      addSlice(parent, fieldname, access_id);
      return;
    }

    if (what == "isocontour") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto fieldname = in.readString("fieldname");
      auto access_id = in.readInt("access_id");
      addIsoContour(parent, fieldname, access_id);
      return;
    }

    if (what == "kdquery") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto fieldname = in.readString("fieldname");
      auto access_id = in.readInt("access_id");
      addKdQuery(parent, fieldname, access_id);
      return;
    }

    if (what == "dataset") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto url = in.readString("url");
      addDataset(parent, url, in.childs.empty() ? StringTree() : *in.getFirstChild());
      return;
    }

    if (what == "modelview") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto insert = in.readBool("insert");
      addModelView(parent, insert);
      return;
    }

    if (what == "scripting") {
      auto parent = findNodeByUUID(in.readString("parent"));
      addScripting(parent);
      return;
    }

    if (what == "cpu_transfer_function") {
      auto parent = findNodeByUUID(in.readString("parent"));
      addCpuTransferFunction(parent);
      return;
    }

    if (what == "palette") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto palette = in.readString("palette");
      addPalette(parent, palette);
      return;
    }

    if (what == "statistics") {
      auto parent = findNodeByUUID(in.readString("parent"));
      addStatistics(parent);
      return;
    }

    if (what == "group") {
      auto name = in.readString("name");
      auto parent = findNodeByUUID(in.readString("parent"));
      addGroup(parent, name);
      return;
    }

    if (what == "glcamera") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto glcamera = GLCamera::decode(*in.getFirstChild());
      addGLCamera(parent, glcamera);
      return;
    }

    if (what == "render") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto palette = in.readString("palette");
      addRender(parent, palette);
      return;
    }

    if (what == "ospray") {
      auto parent = findNodeByUUID(in.readString("parent"));
      auto palette = in.readString("palette");
      addOSPRay(parent, palette);
      return;
    }

    if (what == "kdrender") {
      auto parent = findNodeByUUID(in.readString("parent"));
      addKdRender(parent);
      return;
    }
  }

  if (in.name == "remove")
  {
    auto nodes = StringUtils::split(in.readString("node"));

    if (nodes.size() > 1)
      beginTransaction();

    for (auto node : nodes)
      removeNode(findNodeByUUID(node));

    if (nodes.size() > 1)
      endTransaction();

    return;
  }



  return Model::executeAction(in);
}

///////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<Viewer::Logo> Viewer::openScreenLogo(String key, String default_logo)
{
  String filename = config.readString(key + "/filename");
  if (filename.empty())
    filename = default_logo;

  if (filename.empty())
    return SharedPtr<Logo>();

  auto img = QImage(filename.c_str());
  if (img.isNull()) {
    VisusInfo() << "Failed to load image " << filename;
    return SharedPtr<Logo>();
  }

  auto ret = std::make_shared<Logo>();
  ret->filename = filename;
  ret->tex = std::make_shared<GLTexture>(img);
  ret->tex->envmode = GL_MODULATE;
  ret->pos[0] = StringUtils::contains(key, "Left") ? 0 : 1;
  ret->pos[1] = StringUtils::contains(key, "Bottom") ? 0 : 1;
  ret->opacity = cdouble(config.readString(key + "/alpha", "0.5"));
  ret->border = Point2d(10, 10);
  return ret;
};

////////////////////////////////////////////////////////////
void Viewer::setMinimal()
{
  Viewer::Preferences preferences;
  preferences.panels = "";
  preferences.bHideMenus = true;
  this->setPreferences(preferences);
}

////////////////////////////////////////////////////////////
void Viewer::setFieldName(String value)
{
  if (auto node = this->findNodeByType<FieldNode>())
    node->setFieldName(value);
}

////////////////////////////////////////////////////////////
void Viewer::setScriptingCode(String value)
{
  if (auto node = this->findNodeByType<ScriptingNode>())
    node->setCode(value);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void Viewer::configureFromCommandLine(std::vector<String> args)
{
  for (int I = 1; I<(int)args.size(); I++)
  {
    if (args[I] == "--help")
    {
      VisusInfo() << std::endl
        << "visusviewer help:" << std::endl
        << "   --visus-config <path>                                                  - path to visus.config" << std::endl
        << "   --open <url>                                                           - opens the specified url or .idx volume" << std::endl
        << "   --server                                                               - starts a standalone ViSUS Server on port 10000" << std::endl
        << "   --fullscseen                                                           - starts in fullscreen mode" << std::endl
        << "   --geometry \"<x> <y> <width> <height>\"                                - specify viewer windows size and location" << std::endl
        << "   --zoom-to \"x1 y1 x2 y2\"                                              - set glcamera ortho params" << std::endl
        << "   --network-rcv <port>                                                   - run powerwall slave" << std::endl
        << "   --network-snd <slave_url> <split_ortho> <screen_bounds> <aspect_ratio> - add a slave to a powerwall master" << std::endl
        << "   --split-ortho \"x y width height\"                                     - for taking snapshots" << std::endl
        << "   --internal-network-test-(11|12|14|111)                                 - internal use only" << std::endl
        << std::endl
        << std::endl;

      AppKitModule::detach();
      exit(0);
    }
  }

  typedef Visus::Rectangle2d Rectangle2d;
  String arg_split_ortho;
  String args_zoom_to;

  String open_filename;
  {
    auto configs = config.getAllChilds("dataset");
    if (!configs.empty())
    {
      String dataset_url = configs[0]->readString("url"); VisusAssert(!dataset_url.empty());
      String dataset_name = configs[0]->readString("name", dataset_url);
      open_filename = dataset_name;
    }
  }

  bool bFullScreen = false;
  Rectangle2i geometry(0, 0, 0, 0);
  String fieldname;
  bool bMinimal = false;

  for (int I = 1; I<(int)args.size(); I++)
  {
    if (args[I] == "--open")
    {
      open_filename = args[++I];
    }

    else if (args[I] == "--fullscreen")
    {
      bFullScreen = true;
    }
    else if (args[I] == "--geometry")
    {
      geometry = Rectangle2i(args[++I]);
    }
    else if (args[I] == "--fieldname")
    {
      fieldname = args[++I];
    }
    else if (args[I] == "--minimal")
    {
      bMinimal = true;
    }
    else if (args[I] == "--server")
    {
      auto modvisus = new ModVisus();
      modvisus->configureDatasets();
      this->server = std::make_shared<NetServer>(10000, modvisus);
      this->server->runInBackground();
    }
    else if (args[I] == "--internal-network-test-11")
    {
      auto master = this;
      int X = 50;
      int Y = 50;
      int W = 600;
      int H = 400;
      master->setGeometry(X, Y, W, H);
      Viewer* slave = new Viewer("slave");
      slave->addNetRcv(3333);
      double fix_aspect_ratio = (double)(W) / (double)(H);
      master->addNetSnd("http://localhost:3333", Rectangle2d(0.0, 0.0, 0.5, 1.0), Rectangle2d(X + W, Y, W, H), fix_aspect_ratio);
    }
    else if (args[I] == "--internal-network-test-12")
    {
      auto master = this;
      int W = 1024;
      int H = 768;
      master->setGeometry(300, 300, 600, 400);
      Viewer* slave1 = new Viewer("slave1"); slave1->addNetRcv(3333);
      Viewer* slave2 = new Viewer("slave2"); slave2->addNetRcv(3334);
      int w = W / 2; int ox = 50;
      int h = H; int oy = 50;
      double fix_aspect_ratio = (double)(W) / (double)(H);
      master->addNetSnd("http://localhost:3333", Rectangle2d(0.0, 0.0, 0.5, 1.0), Rectangle2d(ox, oy, w, h), fix_aspect_ratio);
      master->addNetSnd("http://localhost:3334", Rectangle2d(0.5, 0.0, 0.5, 1.0), Rectangle2d(ox + w, oy, w, h), fix_aspect_ratio);
    }
    else if (args[I] == "--internal-network-test-14")
    {
      int W = (int)(0.8*QApplication::desktop()->width());
      int H = (int)(0.8*QApplication::desktop()->height());

      auto master = this;
      master->setGeometry(W / 2 - 300, H / 2 - 200, 600, 400);

      Viewer* slave1 = new Viewer("slave1");  slave1->addNetRcv(3333);
      Viewer* slave2 = new Viewer("slave2");  slave2->addNetRcv(3334);
      Viewer* slave3 = new Viewer("slave3");  slave3->addNetRcv(3335);
      Viewer* slave4 = new Viewer("slave4");  slave4->addNetRcv(3336);

      int w = W / 2; int ox = 50;
      int h = H / 2; int oy = 50;
      double fix_aspect_ratio = (double)(w * 2) / (double)(h * 2);
      master->addNetSnd("http://localhost:3333", Rectangle2d(0.0, 0.0, 0.5, 0.5), Rectangle2d(ox, oy + h, w, h), fix_aspect_ratio);
      master->addNetSnd("http://localhost:3334", Rectangle2d(0.5, 0.0, 0.5, 0.5), Rectangle2d(ox + w, oy + h, w, h), fix_aspect_ratio);
      master->addNetSnd("http://localhost:3335", Rectangle2d(0.5, 0.5, 0.5, 0.5), Rectangle2d(ox + w, oy, w, h), fix_aspect_ratio);
      master->addNetSnd("http://localhost:3336", Rectangle2d(0.0, 0.5, 0.5, 0.5), Rectangle2d(ox, oy, w, h), fix_aspect_ratio);
    }
    else if (args[I] == "--internal-network-test-111")
    {
      auto master = this;
      master->setGeometry(650, 50, 600, 400);

      Viewer* middle = new Viewer("middle");
      middle->addNetRcv(3333);

      Viewer* slave = new Viewer("slave");
      slave->addNetRcv(3334);

      master->addNetSnd("http://localhost:3333", Rectangle2d(0, 0, 1, 1), Rectangle2d(50, 500, 600, 400), 0.0);
      middle->addNetSnd("http://localhost:3334", Rectangle2d(0, 0, 1, 1), Rectangle2d(650, 500, 600, 400), 0.0);
    }
    else if (args[I] == "--network-rcv")
    {
      int port = cint(args[++I]);
      this->addNetRcv(port);
    }
    else if (args[I] == "--network-snd")
    {
      String url = args[++I];
      Rectangle2d split_ortho = Rectangle2d(args[++I]);
      Rectangle2d screen_bounds = Rectangle2d(args[++I]);
      double fix_aspect_ratio = cdouble(args[++I]);
      this->addNetSnd(url, split_ortho, screen_bounds, fix_aspect_ratio);
    }
    else if (args[I] == "--split-ortho")
    {
      arg_split_ortho = args[++I];
    }
    else if (args[I] == "--zoom-to")
    {
      args_zoom_to = args[++I];
    }
    //last argment could be a filename. This facilitates OS-initiated launch (e.g. opening a .idx)
    else if (I == (args.size() - 1) && !StringUtils::startsWith(args[I], "--"))
    {
      open_filename = args[I];
    }
  }

  if (!open_filename.empty())
    this->open(open_filename);

  if (!fieldname.empty())
    this->setFieldName(fieldname);

  if (bMinimal)
    this->setMinimal();


  if (bFullScreen)
    this->showFullScreen();

  else if (geometry.width>0 && geometry.height>0)
    this->setGeometry(geometry.x, geometry.y, geometry.width, geometry.height);

  if (!arg_split_ortho.empty())
  {
    auto split_ortho = Rectangle2d(arg_split_ortho);

    if (auto glcamera = this->getGLCamera())
    {
      double W = glcamera->getViewport().width;
      double H = glcamera->getViewport().height;

      GLOrthoParams ortho_params = glcamera->getOrthoParams().withAspectRatio(W,H);

      ortho_params = ortho_params.split(split_ortho);
      glcamera->setOrthoParams(ortho_params);
    }
  }

  if (!args_zoom_to.empty())
  {
    auto world_box = this->getWorldBox();

    if (auto glcamera = this->getGLCamera())
    {
      GLOrthoParams ortho_params = glcamera->getOrthoParams();

      double x1 = 0, y1 = 0, x2 = 1.0, y2 = 1.0;
      std::istringstream parse(args_zoom_to);
      parse >> x1 >> y1 >> x2 >> y2;

      auto p1 = world_box.getAlphaPoint(PointNd(x1, y1, 0)).toPoint2();
      auto p2 = world_box.getAlphaPoint(PointNd(x2, y2, 0)).toPoint2();

      ortho_params = GLOrthoParams(p1[0], p2[0], p1[1], p2[1], ortho_params.zNear, ortho_params.zFar);

      VisusInfo() << std::fixed << "Setting"
        << " ortho_params("
        << ortho_params.left << " " << ortho_params.right << " "
        << ortho_params.bottom << " " << ortho_params.top << " "
        << ortho_params.zNear << " " << ortho_params.zFar << ")"
        << " world_box("
        << world_box.p1[0] << " " << world_box.p1[1] << " " << world_box.p1[2] << " "
        << world_box.p2[0] << " " << world_box.p2[1] << " " << world_box.p2[2] << ")";


      double W = glcamera->getViewport().width;
      double H = glcamera->getViewport().height;
      ortho_params=ortho_params.withAspectRatio(W,H);

      glcamera->setOrthoParams(ortho_params);
    }
  }
}

////////////////////////////////////////////////////////////
void Viewer::internalFlushMessages()
{
  auto log = this->getLog();

  if (!log)
    return;

  std::vector<String> messages;
  {
    ScopedLock lock(this->log.lock);
    messages = this->log.messages;
    this->log.messages.clear();
  }

  for (auto msg : messages)
  {
    this->log.fstream << msg;
    log->moveCursor(QTextCursor::End);
    log->setTextColor(QColor(0, 0, 0));
    log->insertPlainText(msg.c_str());
    log->moveCursor(QTextCursor::End);
  }
}

////////////////////////////////////////////////////////////
void Viewer::New()
{
  setDataflow(std::make_shared<Dataflow>());
  addNode(dataflow->createNode<Node>("World"));
  clearHistory();
}

////////////////////////////////////////////////////////////
void Viewer::moveNode(Node* dst, Node* src, int index)
{
  if (!dataflow->canMoveNode(dst, src))
    return;

  beginUpdate(
    StringTree("moveNode").write("dst", getUUID(dst)             ).write("src", getUUID(src)).write("index",                   cstring(index)),
    StringTree("moveNode").write("dst", getUUID(src->getParent())).write("src", getUUID(src)).write("index", cstring(src->getIndexInParent())));
  {
    dataflow->moveNode(dst, src, index);
  }
  endUpdate();

  postRedisplay();
}

//////////////////////////////////////////////////////////////////////
void Viewer::enableSaveSession()
{
  save_session_timer.reset(new QTimer());
  
  String filename = config.readString("Configuration/VisusViewer/SaveSession/filename", KnownPaths::VisusHome.getChild("viewer_session.xml"));
  
  int every_sec    =cint(config.readString("Configuration/VisusViewer/SaveSession/sec","60")); //1 min!

  //make sure I create a unique filename
  String extension=Path(filename).getExtension();
  if (!extension.empty())
    filename=filename.substr(0,filename.size()-extension.size());  
  filename=filename+"."+Time::now().getFormattedLocalTime()+extension;

  VisusInfo() << "Configuration/VisusViewer/SaveSession/filename " << filename;
  VisusInfo() << "Configuration/VisusViewer/SaveSession/sec " << every_sec;

  connect(save_session_timer.get(),&QTimer::timeout,[this,filename](){
    save(filename,/*bSaveHistory*/false,/*bShowDialogs*/false);
  });

  if (every_sec>0 && !filename.empty())
    save_session_timer->start(every_sec*1000);
}

//////////////////////////////////////////////////////////////////////
void Viewer::idle()
{
  this->dataflow->dispatchPublishedMessages();

  int   thread_nrunning   = ApplicationStats::num_threads;
  int   thread_pool_njobs = ApplicationStats::num_cpu_jobs;
  int   netservice_njobs  = ApplicationStats::num_net_jobs;

  bool bWasRunning = running.value?true:false;
  bool& bIsRunning = running.value ;
  bIsRunning = thread_pool_njobs || netservice_njobs;

  if (bWasRunning!=bIsRunning)
  {
    if (!bIsRunning)
    {
      running.enlapsed=running.t1.elapsedSec();
      //QApplication::restoreOverrideCursor();
    }
    else
    {
      running.t1=Time::now();
      ApplicationStats::io.reset();
      ApplicationStats::net.reset();
      //QApplication::setOverrideCursor(Qt::BusyCursor);
    }
  }

  std::ostringstream out;

  if (running.value)
    out << "Working. "<<"TJOB(" << thread_pool_njobs << ") "<<"NJOB(" << netservice_njobs    << ") ";
  else
    out << "Ready runtime(" <<running.enlapsed<<"sec ";

  out <<"nthreads(" << thread_nrunning   << ") ";

  out << "IO("
    << StringUtils::getStringFromByteSize((Int64)ApplicationStats::io.nopen ) << "/"
    << StringUtils::getStringFromByteSize((Int64)ApplicationStats::io.rbytes ) << "/"
    << StringUtils::getStringFromByteSize((Int64)ApplicationStats::io.wbytes) << ") ";

  out << "NET("
    << StringUtils::getStringFromByteSize((Int64)ApplicationStats::net.nopen ) << "/"
    << StringUtils::getStringFromByteSize((Int64)ApplicationStats::net.rbytes ) << "/"
    << StringUtils::getStringFromByteSize((Int64)ApplicationStats::net.wbytes) << ") ";

  out << "RAM("
    << StringUtils::getStringFromByteSize(RamResource::getSingleton()->getVisusUsedMemory()) + "/"
    << StringUtils::getStringFromByteSize(RamResource::getSingleton()->getOsUsedMemory()) + "/"
    << StringUtils::getStringFromByteSize(RamResource::getSingleton()->getOsTotalMemory()) << ") ";

  //this seems to slow down OpenGL a lot!
#if 0
  out << "GPU("
    << StringUtils::getStringFromByteSize(GLInfo::getSingleton()->getVisusUsedMemory()) + "/"
    << StringUtils::getStringFromByteSize(GLInfo::getSingleton()->getGpuUsedMemory()) + "/"
    << StringUtils::getStringFromByteSize(GLInfo::getSingleton()->getOsTotalMemory()) << ") ";
#endif

  statusBar()->showMessage(out.str().c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int Viewer::getWorldDimension() const
{
  int ret=0;
  for (auto node : getNodes())
  {
    if (auto dataset_node=dynamic_cast<const DatasetNode*>(node))
    {
      if (Dataset* dataset=dataset_node->getDataset().get()) 
        ret=std::max(ret,dataset->getPointDim());
    }
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BoxNd Viewer::getWorldBox() const
{
  return getPosition(getRoot()).toAxisAlignedBox();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Position Viewer::getPosition(Node* node,bool bRecursive) const
{
  VisusAssert(node);

  //special case for QueryNode: its content is really its bounds
  if (auto query=dynamic_cast<QueryNode*>(node))
    return query->getPosition();

  //NOTE: the result in in local geometric coordinate system of node 
  {
    Position ret=node->getPosition();
    if (ret.valid()) 
      return ret;
  }

  auto T = Matrix::identity(4);

  //modelview_node::modelview is used only if it's NOT recursive call
  //stricly speaking , a transform node has as content its childs
  if (bRecursive)
  {
    if (auto modelview_node=dynamic_cast<ModelViewNode*>(node))
      T=modelview_node->getModelview();
  }

  std::vector<Node*> childs=node->getChilds();

  if (childs.empty())
  {
    return Position::invalid();
  }
  if (childs.size()==1)
  {
    return Position(T, getPosition(childs[0],true));
  }
  else
  {
    auto box= BoxNd::invalid();
    for (auto child : childs)
    {
      Position child_bounds= getPosition(child,true);
      if (child_bounds.valid())
        box=box.getUnion(child_bounds.toAxisAlignedBox());
    }
    return Position(T,box);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Position Viewer::computeNodeToNode(Node* dst,Node* src) const
{
  VisusAssert(dst && src && dst!=src);
  
  Position bounds= getPosition(src);

  bool bAlreadySimplified=false;
  std::deque<Node*> src2root;
  for (Node* cursor=src;cursor;cursor=cursor->getParent())
  {
    if (cursor==dst)
    {
      bAlreadySimplified=true;
      break;
    }
    src2root.push_back(cursor);
  }

  std::deque<Node*> root2dst;
  if (!bAlreadySimplified)
  {
    for (Node* cursor=dst;cursor;cursor=cursor->getParent())
      root2dst.push_front(cursor);

    //symbolic simplification (i.e. if I traverse the same node back and forth)
    while (!src2root.empty() && !root2dst.empty() && src2root.back()==root2dst.front())
    {
      src2root.pop_back();
      root2dst.pop_front();
    }
  }

  for (auto it=src2root.begin();it!=src2root.end();it++)
  {
    if (auto modelview_node=dynamic_cast<ModelViewNode*>(*it))
      bounds=Position(modelview_node->getModelview() , bounds);
  }

  for (auto it=root2dst.begin();it!=root2dst.end();it++) 
  {
    if (auto modelview_node=dynamic_cast<ModelViewNode*>(*it))
      bounds=Position(modelview_node->getModelview().invert() , bounds);
  }

  return bounds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Frustum Viewer::computeNodeToScreen(Frustum frustum,Node* node) const
{
  for (auto it : node->getPathFromRoot())
  {
    if (auto modelview_node=dynamic_cast<ModelViewNode*>(it))
    {
      auto T=modelview_node->getModelview();
      frustum.multModelview(T);
    }
  }

  return frustum;
}

//////////////////////////////////////////////////////////////////////
Position Viewer::computeQueryBounds(QueryNode* query_node) const
{
  return computeNodeToNode(query_node->getDatasetNode(),query_node);
}

////////////////////////////////////////////////////////////
Node* Viewer::findPick(Node* node,Point2d screen_point,bool bRecursive,double* out_distance) const
{
  if (!node)
    return nullptr;

  Node*  ret=nullptr;
  double best_distance=NumericLimits<double>::highest();
    
  //I allow the picking of only queries
  if (QueryNode* query=dynamic_cast<QueryNode*>(node))
  {
    Frustum  node_to_screen = computeNodeToScreen(getGLCamera()->getCurrentFrustum(),node);
    Position node_bounds  = getPosition(node);

    double query_distance= node_to_screen.computeDistance(node_bounds,screen_point,/*bUseFarPoint*/false);
    if (query_distance>=0)
    {
      ret=query;
      best_distance=query_distance;
    }
  }

  if (bRecursive)
  {
    for (auto child : node->getChilds())
    {
      double local_distance;
      if (Node* local_pick=findPick(child,screen_point,bRecursive,&local_distance))
      {
        if (local_distance<best_distance)
        {
          ret=local_pick;
          best_distance=local_distance;
        }
      }
    }
  }

  if (ret && out_distance)
    *out_distance=best_distance;

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
void Viewer::beginFreeTransform(QueryNode* query_node)
{
  //NOTE: this is different from query->getPositionInDatasetNode()
  Position bounds=query_node->getPosition();
  if (!bounds.valid())
  {
    free_transform.reset();
    postRedisplay();
    return;
  }

  if (!free_transform)
  {
    free_transform=std::make_shared<FreeTransform>();

    //whenever free transform change....
    free_transform->objectChanged.connect([this,query_node](Position query_bounds)
    {
      auto T  = query_bounds.getTransformation();
      auto box= query_bounds.getBoxNd().withPointDim(3);

      TRSMatrixDecomposition trs(T);

      if (trs.rotate.getAngle()==0)
      {
        T=Matrix::identity(4);
        for (int I=0;I<3;I++)
        {
          box.p1[I]=box.p1[I]*trs.scale[I]+trs.translate[I];
          box.p2[I]=box.p2[I]*trs.scale[I]+trs.translate[I];
        }
      }
      else
      {
        T=Matrix::translate(trs.translate)*Matrix::rotate(trs.rotate);
        for (int I=0;I<3;I++)
        {
          box.p1[I]=box.p1[I]*trs.scale[I];
          box.p2[I]=box.p2[I]*trs.scale[I];
        }
      }

      query_bounds=Position(T,box);
      query_node->setPosition(query_bounds);
      free_transform->setObject(query_bounds);
    });

  }

  free_transform->setObject(bounds);
  postRedisplay();
}

///////////////////////////////////////////////////////////////////////////////
void Viewer::beginFreeTransform(ModelViewNode* modelview_node)
{
  auto T=modelview_node->getModelview();
  auto bounds= getPosition(modelview_node);

  if (!T.valid() || !bounds.valid()) 
  {
    free_transform.reset();
    postRedisplay();
    return;
  }

  if (!free_transform)
  {
    free_transform=std::make_shared<FreeTransform>();

    free_transform->objectChanged.connect([this,modelview_node,bounds](Position obj)
    {
      auto T=obj.getTransformation() * bounds.getTransformation().invert();
      modelview_node->setModelview(T);
    });
  }

  free_transform->setObject(Position(T,bounds));
  postRedisplay();
}

////////////////////////////////////////////////////////////
void Viewer::endFreeTransform() {
  free_transform.reset();
  postRedisplay();
}

////////////////////////////////////////////////////////////
void Viewer::dataflowBeforeProcessInput(Node* node)
{
  //screen dependent (i.e. viewdep) Need to fix it considering also the tree and transformation nodes
  if (auto query_node=dynamic_cast<QueryNode*>(node))
  {
    //overwrite the query_bounds, need to actualize it to the dataset since QueryNode works in dataset reference space
    auto query_bounds=computeQueryBounds(query_node);
    query_node->setQueryBounds(query_bounds);

    //overwrite the viewdep frustum, since QueryNode works in dataset reference space
    //NOTE: using the FINAL frusutm
    auto node_to_screen=computeNodeToScreen(getGLCamera()->getFinalFrustum(),query_node->getDatasetNode());
    query_node->setNodeToScreen(node_to_screen);
  }
}

////////////////////////////////////////////////////////////
void Viewer::dataflowAfterProcessInput(Node* node)
{
  if (dynamic_cast<GLObject*>(node))
  {
    if (this->getSelection()==node)
      dropSelection();
    postRedisplay();
  }
}

//////////////////////////////////////////////////////////////////////
void Viewer::setDataflow(SharedPtr<Dataflow> value)
{
  //unbind
  if (this->dataflow)
  {
    free_transform.reset();

    detachGLCamera();

    Utils::remove(this->dataflow->listeners,this);

    this->save_session_timer.reset();
    this->idle_timer.reset();

    this->widgets.tabs = nullptr;
    this->widgets.treeview = nullptr;
    this->widgets.frameview = nullptr;
    this->widgets.glcanvas = nullptr;

    this->setCentralWidget(nullptr);
    this->setStatusBar(new QStatusBar());

    //remove all dock widgets
    auto dock_widgets = findChildren<QDockWidget*>();
    for (auto dock_widget : dock_widgets)
    {
      if (dock_widget->widget() == widgets.log)
      {
        widgets.log->show();
        continue;
      }
      
      removeDockWidget(dock_widget);

      //it seams they are already deallocated
      #if 0
      delete dock_widget;
      #endif
    }
  }

  this->dataflow=value;

  //forward to netsnd (important to do here...)
  if (!netsnd.empty())
  {
#if 1
    VisusAssert(false);
#else
    UniquePtr<BindModel> bind_model(new BindModel("BindModel",model,false));
    for (auto connection : netsnd)
      connection->sendNetMessage(bind_model.get());
#endif
  }

  //bind
  if (this->dataflow)
  {
    this->dataflow->listeners.push_back(this);

    setWindowTitle(preferences.title.c_str());

    if (preferences.bHideMenus)
      widgets.toolbar->hide();
    else
      widgets.toolbar->show();

    if (preferences.screen_bounds.valid())
      setGeometry(QUtils::convert<QRect>(preferences.screen_bounds));

    widgets.glcanvas=createGLCanvas();

    //I want to show only the GLCanvas
    if (preferences.panels.empty())
    {
      widgets.log->hide();
      setCentralWidget(widgets.glcanvas);
    }
    else
    {
      widgets.frameview = new DataflowFrameView(this->dataflow.get());
      widgets.treeview = createTreeView();

      //central
      widgets.tabs=new QTabWidget();
      widgets.tabs->addTab(widgets.glcanvas,"GLCanvas");
      widgets.tabs->addTab(widgets.frameview,"Dataflow");
      setCentralWidget(widgets.tabs);

      auto dock = new QDockWidget("Explorer");
      dock->setWidget(widgets.treeview);
      addDockWidget(Qt::LeftDockWidgetArea, dock);
    }

    if (auto glcamera_node= findNodeByType<GLCameraNode>())
      attachGLCamera(glcamera_node->getGLCamera());

    enableSaveSession();

    //timer
    this->idle_timer.reset(new QTimer());
    connect(this->idle_timer.get(), &QTimer::timeout, [this]() {
      idle();
    });
    this->idle_timer->start(1000/20);

    this->refreshData();
    this->postRedisplay();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Viewer::reloadVisusConfig(bool bChooseAFile)
{
  if (bChooseAFile)
  {
    static String last_dir(KnownPaths::VisusHome.toString());
    String filename = cstring(QFileDialog::getOpenFileName(nullptr, "Choose a file to open...", last_dir.c_str(), "*"));
    if (filename.empty()) return;
    last_dir = Path(filename).getParent();
    config.load(filename);
  }
  else
  {
    config.reload();
  }

  widgets.toolbar->bookmarks_button->setMenu(createBookmarks());
}
 
//////////////////////////////////////////////////////////////////////
bool Viewer::open(String url,Node* parent,bool bShowUrlDialogIfNeeded)
{
  if (url.empty())
  {
    if (bShowUrlDialogIfNeeded)
    {
      static String last_url("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1");
      url=cstring(QInputDialog::getText(this,"Enter the url:","",QLineEdit::Normal,last_url.c_str()));
      if (url.empty()) return false;
      last_url=url;
    }
    else
    {
      static String last_filename="";
      url=cstring(QFileDialog::getOpenFileName(nullptr,"Choose a file to open...",last_filename.c_str(),
                                 "All supported (*.idx *.midx *.gidx *.obj *.xml *.config *.scn);;IDX (*.idx *.midx *.gidx);;OBJ (*.obj);;XML files (*.xml *.config *.scn)"));
      
      if (url.empty()) return false;
      last_filename=url;
      url=StringUtils::replaceAll(url,"\\","/");
      if (!StringUtils::startsWith(url,"/")) url="/"+url;
      url="file://" + url;
    }
  }

  VisusAssert(!url.empty());

  //config means a visus.config: merge bookmarks and load first entry (ignore configuration)
  if (StringUtils::endsWith(url,".config"))
  {
    StringTree stree=StringTree::fromString(Utils::loadTextDocument(url));
    if (!stree.valid())
    {
      VisusAssert(false);
      return false;
    }

    for (int i=0;i<stree.getNumberOfChilds();i++)
      config.addChild(stree.getChild(i));

    //load first bookmark from new config
    auto children=stree.getAllChilds("dataset");
    if (!children.empty())
    {
      url=children[0]->readString("name",children[0]->readString("url"));VisusAssert(!url.empty());
      return this->open(url,parent,false);
    }

    return true;
  }

  //xml means a Viewer dataflow
  if (StringUtils::endsWith(url,".xml"))
  {
    StringTree in=StringTree::fromString(Utils::loadTextDocument(url));
    if (!in.valid())
    {
      VisusAssert(false);
      return false;
    }

    setDataflow(std::make_shared<Dataflow>());
    clearHistory();

    try
    {
      this->readFrom(in);
    }
    catch (std::exception ex)
    {
      VisusAssert(false);
      return false;
    }

    //clearHistory();
#ifdef VISUS_DEBUG
    enableLog("visusviewer.log.txt");
#endif

    VisusInfo() << "open(" << url << ") done";
    widgets.treeview->expandAll();
    refreshActions();
    return true;
  }

  //gidx means group of idx: open them all
  //scrgiorgio: does gidx makes sense with midx?
#if 0
  if (StringUtils::endsWith(url, ".gidx"))
  {
    StringTree stree = StringTree::fromString(Utils::loadTextDocument(url));
    if (!stree.valid())
    {
      VisusAssert(false);
      return false;
    }

    for (auto child : stree.childs)
    {
      if (child->name != "dataset")
        continue;

      String dataset(child->readString("url"));
      bool success = this->open(dataset, child == stree.childs.front() ? parent : getRoot(), false);
      if (!success)
        VisusWarning() << "Unable to open " << dataset << " from " << url;
    }

    widgets.treeview->expandAll();
    refreshActions();
    VisusInfo() << "open(" << url << ") done";
    return true;
  }
#endif

  //try to open a dataset
  auto dataset = LoadDatasetEx(url,this->config);
  if (!dataset)
  {
    QMessageBox::information(this, "Error", (StringUtils::format() << "open file(" << url << +") failed.").str().c_str());
    return false;
  }

  if (!parent) {
    New();
    parent = dataflow->getRoot();
  }

  beginUpdate(
    StringTree("open").write("url",dataset->getUrl()).write("parent",getUUID(parent)),
    Transaction());
  {
    //make sure there is a camera
    auto glcamera=getGLCamera();
    if (!glcamera)
    {
      if (dataset->getPointDim() ==3)
        glcamera=std::make_shared<GLLookAtCamera>();
      else
        glcamera=std::make_shared<GLOrthoCamera>();

      addGLCamera(parent, glcamera);
    }

    addDataset(parent, dataset);

    glcamera->guessPosition(getWorldBox());

    if (dataset && StringUtils::contains(dataset->getConfig().readString("mirror"), "x"))
      glcamera->mirror(0);

    if (dataset && StringUtils::contains(dataset->getConfig().readString("mirror"), "y"))
      glcamera->mirror(1);

  }
  endUpdate();

  if (widgets.treeview)
    widgets.treeview->expandAll();
  
  refreshActions();
  VisusInfo()<<"open("<<url<<") done";
  return true;
}

//////////////////////////////////////////////////////////////////////
bool Viewer::save(String url,bool bSaveHistory,bool bShowDialogs)
{ 
  if (url.empty() && bShowDialogs)
  {
    static String last_dir(KnownPaths::VisusHome.toString());
    url=cstring(QFileDialog::getSaveFileName(nullptr,"Choose a file to save...",last_dir.c_str(),"*.xml"));
    if (url.empty()) return false;
    last_dir=Path(url).getParent();
  }

  //add default extension
  if (Path(url).getExtension().empty())
    url=url+".xml";

  try
  {
    if (bSaveHistory)
    {
      auto out = getHistory();
      out.name = "Viewer";
      out.writeString("version", cstring(ApplicationInfo::version));
      out.writeString("git_revision", ApplicationInfo::git_revision);
    }
    else
    {
      StringTree out("Viewer");
      this->writeTo(out);
      if (!Utils::saveTextDocument(url, out.toString()))
        ThrowException("saveTextDocument failed");
    }
  }
  catch (std::exception ex)
  {
    if (bShowDialogs) 
    {
      String errormsg=StringUtils::format()<<"Failed to save file " + url + "error("+ex.what()+")";
      QMessageBox::information(this,"Error",errormsg.c_str());
    }
    return false;
  }

  this->last_saved_filename=url;

  if (bShowDialogs) 
  {
    String errormsg=StringUtils::format()<<"File " + url+ " saved";
    QMessageBox::information(this,"Info",errormsg.c_str());
  }

  return true;
}
  
////////////////////////////////////////////////////////////
void Viewer::setAutoRefresh(AutoRefresh new_value)  {

  auto& old_value = this->auto_refresh;

  //useless call
  if (old_value.msec == new_value.msec && old_value.enabled == new_value.enabled)
    return;

  beginUpdate(
    StringTree("set").write("target_id", "auto_refresh").write("enabled", new_value.enabled).write("msec", new_value.msec),
    StringTree("set").write("target_id", "auto_refresh").write("enabled", old_value.enabled).write("msec", old_value.msec));
  {
    old_value = new_value;
    widgets.toolbar->auto_refresh.check->setChecked(new_value.enabled);
    widgets.toolbar->auto_refresh.msec->setText(cstring(new_value.msec).c_str());
  }
  endUpdate();

  if (auto_refresh.enabled && auto_refresh.msec)
  {
    this->auto_refresh_timer = std::make_shared<QTimer>();
    QObject::connect(this->auto_refresh_timer.get(), &QTimer::timeout, [this]() {
      refreshData();
    });
    this->auto_refresh_timer->start(this->auto_refresh.msec);
  }

}

////////////////////////////////////////////////////////////////////
void Viewer::dropProcessing()
{
  beginUpdate(
    StringTree("dropProcessing"),
    StringTree("dropProcessing"));
  {
    dataflow->abortProcessing();
    dataflow->joinProcessing();
  }
  endUpdate();

  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::setMouseDragging(bool new_value)
{
  Model::setProperty("mouse_dragging",this->mouse_dragging,new_value);
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
void Viewer::scheduleMouseDragging(bool value, int msec)
{
  //stop dragging: postpone a little the end-drag event for the camera
  widgets.glcanvas->mouse_timer.reset(new QTimer());
  connect(widgets.glcanvas->mouse_timer.get(), &QTimer::timeout, [this, value] {
    widgets.glcanvas->mouse_timer.reset();
    setMouseDragging(value);
  });
  widgets.glcanvas->mouse_timer->start(msec);
}


////////////////////////////////////////////////////////////////////
void Viewer::setSelection(Node* new_value)
{
  auto old_value=getSelection();
  if (old_value == new_value)
    return;

  beginUpdate(
    StringTree("setSelection").write("node", getUUID(new_value)),
    StringTree("setSelection").write("node", getUUID(old_value)));
  {
    dataflow->setSelection(new_value);
  }
  endUpdate();

  //in case there is an old free transform going...
  endFreeTransform();

  if (auto query_node=dynamic_cast<QueryNode*>(new_value))
    beginFreeTransform(query_node);
    
  else if (auto modelview_node=dynamic_cast<ModelViewNode*>(new_value))
    beginFreeTransform(modelview_node);

  refreshActions();

  postRedisplay();
}

//////////////////////////////////////////////////////////
void Viewer::setNodeName(Node* node,String new_value)
{
  if (!node) return;
  node->setName(new_value);
  postRedisplay();
}

//////////////////////////////////////////////////////////
void Viewer::setNodeVisible(Node* node,bool new_value)
{
  if (!node)
    return;

  auto old_value = node->isVisible();
  if (old_value == new_value)
    return;

  beginUpdate(
    StringTree("showNode").writeString("node", getUUID(node)).write("value", new_value),
    StringTree("showNode").writeString("node", getUUID(node)).write("value", old_value));
  {
    dropProcessing();
    for (auto it : node->breadthFirstSearch())
      node->setVisible(new_value);
  }
  endUpdate();

  refreshActions();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::addNode(Node* parent,Node* node,int index)
{
  if (!node)
    return;

  if (dataflow->containsNode(node))
    return;

  node->begin_update.connect([this,node](){
    beginUpdate(
      StringTree("__change__"),
      StringTree("__change__"));
  });

  node->end_update.connect([this,node](){
    this->topRedo() = createPassThroughAction(node->topRedo(),"nodes/" + getUUID(node));
    this->topUndo() = createPassThroughAction(node->topUndo(),"nodes/" + getUUID(node));

    //if something changes in the query...
    if (auto query_node = dynamic_cast<QueryNode*>(node))
      refreshData(query_node);

    else if (auto modelview_node=dynamic_cast<ModelViewNode*>(node))
      refreshData(modelview_node);

    endUpdate();
  });

  dropSelection();

  beginTransaction();
  {
    beginUpdate(
      StringTree("add").write("what","node").write("parent",getUUID(parent)).write("index",index).addChild(EncodeObject(*node)),
      StringTree("remove").write("node",getUUID(node)));
    {
      dataflow->addNode(parent, node, index);
    }
    endUpdate();
  }
  endTransaction();
  
  if (auto glcamera_node=dynamic_cast<GLCameraNode*>(node))
    attachGLCamera(glcamera_node->getGLCamera());
  
  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::removeNode(Node* NODE)
{
  if (!NODE)
    return;

  beginUpdate(
    StringTree("remove").write("node",getUUID(NODE)), 
    Transaction());
  {
    dropProcessing();
    dropSelection();

    for (auto node : NODE->reversedBreadthFirstSearch())
    {
      VisusAssert(node->getChilds().empty());

      if (auto glcamera_node = dynamic_cast<GLCameraNode*>(node))
        detachGLCamera();

      //disconnect inputs
      for (auto input : node->inputs)
      {
        DataflowPort* iport = input.second; VisusAssert(iport->outputs.empty());//todo multi dataflow
        while (!iport->inputs.empty())
        {
          auto oport = *iport->inputs.begin();
          disconnectPorts(oport->getNode(), oport->getName(), iport->getName(), iport->getNode());
        }
      }

      //disconnect outputs
      for (auto output : node->outputs)
      {
        auto oport = output.second; VisusAssert(oport->inputs.empty());//todo multi dataflow
        while (!oport->outputs.empty())
        {
          auto iport = *oport->outputs.begin();
          disconnectPorts(oport->getNode(), oport->getName(), iport->getName(), iport->getNode());
        }
      }

      VisusAssert(node->isOrphan());

      Utils::push_front(topUndo().childs,
        std::make_shared<StringTree>(
          StringTree("add")
            .write("what", "node")
            .write("parent", getUUID(node->getParent()))
            .write("index", cstring(node->getIndexInParent())).addChild(EncodeObject(*node))));

      dataflow->removeNode(node);
    }

    autoConnectPorts();
  }
  endUpdate();

  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::connectPorts(Node* from,String oport,String iport,Node* to)
{
  beginUpdate(
    StringTree("connectPorts"   ).write("from", getUUID(from)).write("oport", oport).write("iport", iport).write("to", getUUID(to)),
    StringTree("disconnectPorts").write("from", getUUID(from)).write("oport", oport).write("iport", iport).write("to", getUUID(to)));
  {
    dataflow->connectPorts(from, oport, iport, to);
  }
  endUpdate();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::disconnectPorts(Node* from,String oport,String iport,Node* to)
{
  beginUpdate(
    StringTree("disconnectPorts").write("from", getUUID(from)).write("oport", oport).write("iport", iport).write("to", getUUID(to)),
    StringTree("connectPorts"   ).write("from", getUUID(from)).write("oport", oport).write("iport", iport).write("to", getUUID(to)));
  {
    dataflow->disconnectPorts(from, oport, iport, to);
  }
  endUpdate();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////////
void Viewer::autoConnectPorts()
{
  beginTransaction();

  for (auto node : getRoot()->breadthFirstSearch())
  {
    for (auto input : node->inputs)
    {
      DataflowPort* iport = input.second;

      //already connected? skip it!
      if (iport->isConnected())
        continue;

      DataflowPort* oport = nullptr;

      //I have a child with no input ports and an auto connected output port
      for (auto child : node->getChilds())
      {
        if (child->inputs.empty() && (oport = child->getOutputPort(iport->getName())))
          break;
      }

      //going up including (up) brothers
      if (!oport)
      {
        for (Node* cursor = node->goUpIncludingBrothers(); cursor; cursor = cursor->goUpIncludingBrothers())
          if ((oport = cursor->getOutputPort(iport->getName())))
            break;
      }

      if (oport)
        connectPorts(oport->getNode(), oport->getName(), iport->getName(), iport->getNode());
    }
  }

  endTransaction();

  postRedisplay();
}

/////////////////////////////////////////////////////////////////
void Viewer::refreshData(Node* node)
{
  beginUpdate(
    StringTree("refreshData").write("node", getUUID(node)),
    StringTree("refreshData").write("node", getUUID(node)));

  if (node)
  {
    if (auto query_node=dynamic_cast<QueryNode*>(node))
    {
      query_node->setPosition(query_node->getPosition(),/*bForce*/true);
    }
    else if (auto modelview_node=dynamic_cast<ModelViewNode*>(node))
    {
      //find all queries, they could have been their relative position in the dataset changed
      for (auto it : modelview_node->breadthFirstSearch())
      {
        if (auto query_node=dynamic_cast<QueryNode*>(it))
        {
          Position query_bounds=computeQueryBounds(query_node);
          if (query_bounds !=query_node->getQueryBounds())
            dataflow->needProcessInput(query_node);
        }
      }
    }
    else
    {
      //nothing to do ?
    }
  }
  else
  {
    for (auto node : getNodes())
    {
      if (auto query=dynamic_cast<QueryNode*>(node))
        dataflow->needProcessInput(query);
    }
  }

  endUpdate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addGroup(Node* parent, String name)
{
  if (!parent)
    parent = getRoot();

  if (name.empty())
  {
    name = cstring(QInputDialog::getText(this, "Insert the group name:", "", QLineEdit::Normal, ""));
    if (name.empty())
      return nullptr;
  }

  auto group_node = new Node(name);
  group_node->setUUID(dataflow->guessNodeUIID("Group"));

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","group").write("parent",getUUID(parent)).write("name",name),
    StringTree("remove").write("node",getUUID(group_node)));
  {
    
    addNode(parent, group_node, /*index*/0);
  }
  endUpdate();

  return group_node;
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
GLCameraNode* Viewer::addGLCamera(Node* parent, SharedPtr<GLCamera> glcamera)
{
  if (!parent)
    parent=getRoot();

  auto glcamera_node=dataflow->createNode<GLCameraNode>("GLCamera", glcamera);

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","glcamera").write("parent", getUUID(parent)).addChild(EncodeObject(*glcamera)),
    StringTree("remove").write("node",getUUID(glcamera_node)));
  {
    addNode(parent, glcamera_node,/*index*/0);
  }
  endUpdate();

  return glcamera_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GLCameraNode* Viewer::addGLCamera(Node* parent, String type)
{
  type = StringUtils::toLower(type);

  if (type == "lookat" || type=="gllookatcamera")
    return addGLCamera(parent, std::make_shared<GLLookAtCamera>());

  if (type == "ortho" || type=="glorthocamera")
    return addGLCamera(parent, std::make_shared<GLOrthoCamera>());

  ThrowException("unknown glcamera type ");
  return nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addRender(Node* parent,String palette)  
{
  if (!parent)
    parent = getRoot();

  VisusReleaseAssert(parent->hasOutputPort("data"));

  auto render_node = dataflow->createNode<RenderArrayNode>("Render");
  auto palette_node= palette.empty()? nullptr : dataflow->createNode<PaletteNode>("Palette", palette);

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","render").write("parent", getUUID(parent)).write("palette", palette),
    StringTree("remove").write("node","render_node"));
  {
    addNode(parent, render_node);

    connectPorts(parent,"data", render_node);

    if (palette_node)
    {
      addNode(render_node, palette_node);
      connectPorts(parent,"data",palette_node);
      connectPorts(palette_node,"palette", render_node);
    }
  }
  endUpdate();

  return render_node;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addOSPRay(Node* parent, String palette)
{
  if (!parent)
    parent = getRoot();

  VisusReleaseAssert(parent->hasOutputPort("data"));

  Node* render_node = dataflow->createNode<OSPRayRenderNode>("OSPrayRender");

  auto palette_node = palette.empty() ? nullptr : dataflow->createNode<PaletteNode>("Palette", palette);

  dropSelection();

  beginUpdate(
    StringTree("add").write("what", "ospray").write("parent", getUUID(parent)).write("palette", palette),
    StringTree("remove").write("node",getUUID(render_node)));
  {
    addNode(parent, render_node);

    connectPorts(parent, "data", render_node);

    if (palette_node)
    {
      addNode(render_node, palette_node);
      connectPorts(parent, "data", palette_node);
      connectPorts(palette_node, "palette", render_node);
    }
  }
  endUpdate();

  return render_node;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
KdRenderArrayNode* Viewer::addKdRender(Node* parent) 
{
  if (!parent)
    parent = getRoot();

  VisusReleaseAssert(parent->hasOutputPort("data"));

  auto render_node = dataflow->createNode<KdRenderArrayNode>("KdRender");
  auto palette_node= dataflow->createNode<PaletteNode>("Palette","GrayOpaque");

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","kdrender").write("parent", getUUID(parent)),
    StringTree("remove").write("node",getUUID(render_node)));
  {
    addNode(parent,render_node);
    addNode(render_node, palette_node);
    connectPorts(palette_node,render_node);
  }
  endUpdate();

  return render_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addVolume(Node* parent, String fieldname, int access_id)
{
  if (!parent)
  {
    parent=findNodeByType<DatasetNode>();
    if (!parent)
      parent=getRoot();
  }

  auto* dataset_node= dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node= findNodeByType<DatasetNode>();
  VisusReleaseAssert(dataset_node);

  auto dataset=dataset_node->getDataset();
  if (!dataset) 
  {
    VisusInfo()<<"Cannot find dataset";
    return nullptr;
  }

  if (fieldname.empty())
    fieldname=dataset->getDefaultField().name;


  auto query_node= dataflow->createNode<QueryNode>("Volume");
  {
    query_node->setVerbose(1);
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setProgression(QueryGuessProgression);
    query_node->setQuality(QueryDefaultQuality);
    query_node->setPosition(dataset_node->getPosition());
  }

  auto field_node= dataflow->createNode<FieldNode>("Field", fieldname);

  auto scripting_node = dataflow->createNode<ScriptingNode>("Scripting");

  auto time_node=dataset_node->findChild<TimeNode*>();
  if (!time_node)
    time_node= dataflow->createNode<TimeNode>("Time",dataset->getDefaultTime(),dataset->getTimesteps());

  beginUpdate(
    StringTree("add").write("what","volume").write("parent",getUUID(parent)).write("fieldname", fieldname).write("access_id", access_id),
    StringTree("remove").write("node",getUUID(query_node)));
  {
    addNode(parent,query_node);
    addNode(query_node, field_node);
    addNode(query_node, scripting_node);
    addNode(query_node, time_node); //this is a nope if time_node already exists

    connectPorts(dataset_node,query_node);
    connectPorts(time_node,query_node);
    connectPorts(field_node,query_node);
    connectPorts(query_node, "data", scripting_node);

    addRender(scripting_node, "GrayTransparent");
  }
  endUpdate();

  return query_node;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addSlice(Node* parent, String fieldname, int access_id)
{
  if (!parent)
  {
    parent = findNodeByType<DatasetNode>();
    if (!parent)
      parent = getRoot();
  }

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNodeByType<DatasetNode>();
  VisusReleaseAssert(dataset_node);

  auto dataset = dataset_node->getDataset();
  if (!dataset)
  {
    VisusInfo() << "Cannot find dataset";
    return nullptr;
  }

  if (fieldname.empty())
    fieldname = dataset->getDefaultField().name;

  auto query_node = dataflow->createNode<QueryNode>("Slice");
  {
    query_node->setVerbose(1);
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setProgression(QueryGuessProgression);
    query_node->setQuality(QueryDefaultQuality);

    if (bool bPointQuery = dataset->getPointDim() == 3)
    {
      auto box = dataset_node->getPosition().getBoxNd().withPointDim(3);
      box.p1[2] = box.p2[2] = box.center()[2];
      query_node->setPosition(Position(dataset_node->getPosition().getTransformation(), box));
    }
    else
    {
      query_node->setPosition(dataset_node->getPosition());
    }
  }

  auto field_node = dataflow->createNode<FieldNode>("fieldnode", fieldname);

  auto scripting_node = dataflow->createNode<ScriptingNode>("Scripting");

  auto time_node = dataset_node->findChild<TimeNode*>();
  if (!time_node)
    time_node = dataflow->createNode<TimeNode>("time", dataset->getDefaultTime(), dataset->getTimesteps());

  beginUpdate(
    StringTree("add").write("what", "slice").write("parent", getUUID(parent)).write("fieldname", fieldname).write("access_id", access_id),
    StringTree("remove").write("node",getUUID(query_node)));
  {
    addNode(parent, query_node);
    addNode(query_node, field_node);
    addNode(query_node, scripting_node);
    addNode(query_node, time_node);

    connectPorts(dataset_node, query_node);
    connectPorts(time_node, query_node);
    connectPorts(field_node, query_node);
    connectPorts(query_node, "data", scripting_node);

    addRender(scripting_node, "GrayOpaque");
  }
  endUpdate();

  return query_node;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addIsoContour(Node* parent, String fieldname, int access_id)
{
  if (!parent)
  {
    parent = findNodeByType<DatasetNode>();
    if (!parent)
      parent = getRoot();
  }

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNodeByType<DatasetNode>();
  VisusReleaseAssert(dataset_node);

  auto dataset = dataset_node->getDataset();
  VisusReleaseAssert(dataset);

  if (fieldname.empty())
    fieldname = dataset->getDefaultField().name;

  auto query_node = dataflow->createNode<QueryNode>("IsoContour");
  {
    query_node->setVerbose(1);
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setProgression(QueryGuessProgression);
    query_node->setQuality(QueryDefaultQuality);
    query_node->setPosition(dataset_node->getPosition());
  }

  auto field_node = dataflow->createNode<FieldNode>("fieldnode", fieldname);

  auto scripting_node = dataflow->createNode<ScriptingNode>("Scripting");

  auto time_node = dataset_node->findChild<TimeNode*>();
  if (!time_node)
    time_node = dataflow->createNode<TimeNode>("time", dataset->getDefaultTime(), dataset->getTimesteps());

  auto build_isocontour = dataflow->createNode<IsoContourNode>("Marching cube");
  {
    double isovalue = 0.0;
    Field field = dataset->getFieldByName(fieldname);
    if (field.dtype.ncomponents() == 1)
    {
      Range range = field.dtype.getDTypeRange();
      isovalue = 0.5 * (range.from + range.to);
    }
    build_isocontour->setIsoValue(isovalue);
  }
  
  auto render_node = dataflow->createNode<IsoContourRenderNode>("Mesh Render");
  {
    GLMaterial material = render_node->getMaterial();
    material.front.diffuse = Color::createFromUint32(0x3c6d3eff);
    material.front.specular = Color::createFromUint32(0xffffffff);
    material.back.diffuse = Color::createFromUint32(0x2a4b70ff);
    material.back.specular = Color::createFromUint32(0xffffffff);
    render_node->setMaterial(material);
  }

  //this is useful if the data coming from the data provider has 2 components, first is used to compute the 
  //marching cube, the second as a second field to color the vertices of the marching cube
  auto palette_node = dataflow->createNode<PaletteNode>("Palette", "GrayOpaque");

  dropSelection();

  beginUpdate(
    StringTree("add").write("what", "isocontour").write("parent", getUUID(parent)).write("fieldname", fieldname).write("access_id", access_id),
    StringTree("remove").write("node", "query_node"));
  {
    addNode(parent, query_node);
    addNode(query_node, field_node);
    addNode(query_node, time_node);
    addNode(query_node, scripting_node);

    addNode(scripting_node, build_isocontour);
    addNode(scripting_node, palette_node);
    addNode(scripting_node, render_node);

    connectPorts(dataset_node, query_node);
    connectPorts(time_node, query_node);
    connectPorts(field_node, query_node);
    connectPorts(query_node, "data", scripting_node);
    connectPorts(scripting_node, "data", build_isocontour);
    connectPorts(build_isocontour, "data", render_node);
    connectPorts(build_isocontour, "data", palette_node); //enable statistics
    connectPorts(palette_node, "palette", render_node);
  }
  endUpdate();

  return query_node;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
KdQueryNode* Viewer::addKdQuery(Node* parent,String fieldname,int access_id) 
{
  if (!parent)
  {
    parent= findNodeByType<DatasetNode>();
    if (!parent)
      parent=getRoot();
  }

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNodeByType<DatasetNode>();
  VisusReleaseAssert(dataset_node);

  auto dataset=dataset_node->getDataset();
  if (!dataset)
  {
    VisusInfo()<<"Cannot find dataset";
    return nullptr;
  }

  if (fieldname.empty())
    fieldname=dataset->getDefaultField().name;

  //KdQueryNode
  auto query_node= dataflow->createNode<KdQueryNode>("KdQuery");
  {
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setQuality(QueryDefaultQuality);
    query_node->setPosition(dataset_node->getPosition());
  }

  //TimeNode
  auto time_node=dataset_node->findChild<TimeNode*>();
  if (!time_node)
    time_node= dataflow->createNode<TimeNode>("time",dataset->getDefaultTime(),dataset->getTimesteps());

  //FieldNode
  auto field_node= dataflow->createNode<FieldNode>("fieldnode");
  {
    field_node->setFieldName(fieldname);
  }

  //render_node
  auto render_node = dataflow->createNode<KdRenderArrayNode>("KdRender");
  auto palette_node= dataflow->createNode<PaletteNode>("Palette","GrayOpaque");

  beginUpdate(
    StringTree("add").write("what","kdquery").write("parent", getUUID(parent)).write("fieldname", fieldname).write("access_id", access_id),
    StringTree("remove").write("node", getUUID(query_node)));
  {
    addNode(parent,query_node);
    addNode(query_node,field_node);
    addNode(query_node,palette_node);
    addNode(query_node,render_node);
    addNode(query_node,time_node);

    connectPorts(dataset_node,query_node);
    connectPorts(time_node,query_node);
    connectPorts(field_node,query_node);
    //connectPorts(query_node,"data",palette_node); this enable statistics 
    connectPorts(palette_node,"palette",render_node);
    connectPorts(query_node,"data",render_node);
  }
  endUpdate();

  return query_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DatasetNode* Viewer::addDataset(Node* parent, SharedPtr<Dataset> dataset)
{
  if (!parent)
    parent=getRoot();

  VisusAssert(dataset);

  auto dataset_node= dataflow->createNode<DatasetNode>("Dataset");
  dataset_node->setName(dataset->getUrl().toString());
  dataset_node->setDataset(dataset);
  dataset_node->setShowBounds(true);

  //time (this is for queries...)
  auto time_node= dataflow->createNode<TimeNode>("time",dataset->getDefaultTime(),dataset->getTimesteps());

  auto config = dataset->getConfig();

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","dataset").write("parent",getUUID(parent)).write("url",dataset->getUrl()).addChild(config),
    StringTree("remove").write("node", getUUID(dataset_node)));
  {
    addNode(parent,dataset_node);
    addNode(dataset_node,time_node);

    String rendertype = StringUtils::toLower(config.readString("rendertype", ""));

    if ((dataset->getKdQueryMode() != KdQueryMode::NotSpecified) || rendertype == "kdrender")
      addKdQuery(dataset_node);

    else if (dataset->getPointDim() == 3)
      addVolume(dataset_node);

    else
      addSlice(dataset_node);

    this->refreshData();
  }
  endUpdate();

  return dataset_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ModelViewNode* Viewer::addModelView(Node* parent,bool insert)
{
  if (!parent)
    parent = getRoot();

  auto modelview_node = dataflow->createNode<ModelViewNode>("Transform");

  beginUpdate(
    StringTree("add").write("what","modelview").write("parent", getUUID(parent)).write("insert", insert),
    insert? Transaction() : StringTree("remove").write("node", getUUID(modelview_node)));
  {
    if (insert)
    {
      Node* A = parent->getParent(); VisusAssert(A);
      Node* B = modelview_node;
      Node* C = parent;
      int index = C->getIndexInParent();
      addNode(A, B, index);
      moveNode(/*dst*/B,/*src*/C);
    }
    else
    {
      addNode(parent, modelview_node);
    }
  }
  endUpdate();

  return modelview_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ScriptingNode* Viewer::addScripting(Node* parent) 
{
  if (!parent)
    parent=getRoot();

  VisusAssert(parent->hasOutputPort("data"));

  auto scripting_node= dataflow->createNode<ScriptingNode>("Scripting");
  scripting_node->setMaxPublishMSec(cint(config.readString("Configuration/ScriptingNode/max_publish_msec", cstring(scripting_node->getMaxPublishMSec()))));

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","scripting").write("parent", getUUID(parent)),
    StringTree("remove"));
  {
    addNode(parent,scripting_node);
    addRender(scripting_node,"GrayOpaque");
    connectPorts(parent,"data",scripting_node);
  }
  endUpdate();

  return scripting_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CpuPaletteNode* Viewer::addCpuTransferFunction(Node* parent) 
{
  if (!parent)
    parent=getRoot();

  VisusAssert(parent->hasOutputPort("data"));

  //guess number of functions
  int num_functions = 1;
  if (DataflowPortValue * last_published = dataflow->guessLastPublished(parent->getOutputPort("data")))
  {
    if (auto last_data = std::dynamic_pointer_cast<Array>(last_published->value))
      num_functions = last_data->dtype.ncomponents();
  }
  
  //CpuPaletteNode
  auto node= dataflow->createNode<CpuPaletteNode>("CPU Palette");
  {
    auto palette = TransferFunction::getDefault("GrayOpaque");
    palette->setNumberOfFunctions(num_functions);
    node->setTransferFunction(palette);
  }

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","cpu_transfer_function").write("parent", getUUID(parent)),
    StringTree("remove").write("node", getUUID(node)));
  {
    addNode(parent, node);
    addRender(node,/*no need for a palette*/"");
    connectPorts(parent,"data", node);
  }
  endUpdate();

  return node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
PaletteNode* Viewer::addPalette(Node* parent,String palette) 
{
  if (!parent)
    parent=getRoot();

  auto palette_node= dataflow->createNode<PaletteNode>("Palette",palette);

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","palette").write("parent", getUUID(parent)).write("palette", palette),
    StringTree("remove").write("node", getUUID(palette_node)));
  {
    addNode(parent,palette_node,1);

    //enable statistics
    if (parent->hasOutputPort("data"))
      connectPorts(parent,"data",palette_node);
  }
  endUpdate();

  return palette_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
StatisticsNode* Viewer::addStatistics(Node* parent)  
{
  if (!parent)
    parent = getRoot();

  VisusReleaseAssert(parent->hasOutputPort("data"));

  auto statistics_node= dataflow->createNode<StatisticsNode>("Statistics");

  dropSelection();

  beginUpdate(
    StringTree("add").write("what","statistics").write("parent", getUUID(parent)),
    StringTree("remove").write("node", getUUID(statistics_node)));
  {
    addNode(parent,statistics_node);
    connectPorts(parent,"data",statistics_node);
  }
  endUpdate();

  return statistics_node;
}

/////////////////////////////////////////////////////////////
void Viewer::writeTo(StringTree& out) const
{
  out.writeString("version", cstring(ApplicationInfo::version));
  out.writeString("git_revision", ApplicationInfo::git_revision);

  //first dump the nodes without parent... NOTE: the first one is always the getRoot()
  auto root = dataflow->getRoot();
  VisusAssert(root == dataflow->getNodes()[0]);
  for (auto node : dataflow->getNodes())
  {
    if (node->getParent()) continue;
    out.addChild(StringTree("add").write("what","node").addChild(EncodeObject(*node)));
  }

  //then the nodes in the tree...important the order! parents before childs
  for (auto node : root->breadthFirstSearch())
  {
    if (node == root) continue;
    VisusAssert(node->getParent());
    out.addChild(StringTree("add")
      .write("what", "node")
      .write("parent", getUUID(node->getParent()))
      .addChild(EncodeObject(*node)));
  }

  //ConnectPorts actions
  for (auto node : dataflow->getNodes())
  {
    for (auto OT = node->outputs.begin(); OT != node->outputs.end(); OT++)
    {
      auto oport = OT->second;
      for (auto IT = oport->outputs.begin(); IT != oport->outputs.end(); IT++)
      {
        auto iport = (*IT);
        out.addChild(StringTree("connectPorts")
          .write("from", getUUID(oport->getNode()))
          .write("oport", oport->getName())
          .write("iport", iport->getName())
          .write("to", getUUID(iport->getNode())));
      }
    }
  }

  //selection
  if (auto selection = getSelection())
    out.addChild(StringTree("setSelection").write("node", getUUID(selection)));
}

/////////////////////////////////////////////////////////////
void Viewer::readFrom(StringTree& in)
{
  double version = cdouble(in.readString("version"));
  String git_revision = in.readString("git_revision");

  for (auto child : in.childs)
  {
    if (child->isHashNode())
      continue;

    this->executeAction(*child);
  }
}



} //namespace Visus


