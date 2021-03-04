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
#include <Visus/ModelViewNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/KdQueryNode.h>
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

static void RedirectLogToViewer(String msg, void* user_data)
{
  auto viewer = (Viewer*)user_data;
  viewer->printInfo(msg);
}

///////////////////////////////////////////////////////////////////////////////////////////////
Viewer::Viewer(String title) : QMainWindow()
{
  if (GLInfo::getSingleton()->getGpuTotalMemory())
  {
    QueryNode::willFitOnGpu = [](Int64 size) {

      auto freemem = GLInfo::getSingleton()->getGpuFreeMemory();
      if ((1.2 * size) < freemem)
        return true;
      PrintInfo("Discarning data since it wont' fit on GPU", StringUtils::getStringFromByteSize(size), "freemem", StringUtils::getStringFromByteSize(freemem));
      return false;
    };
  };

  this->config = *GuiModule::getModuleConfig();

  RedirectLogTo(RedirectLogToViewer, this);

  connect(this, &Viewer::postFlushMessages, this, &Viewer::internalFlushMessages, Qt::QueuedConnection);

  this->log.fstream.open(KnownPaths::VisusHome + "/visus." + Time::now().getFormattedLocalTime()+ ".log");
  VisusAssert(this->log.fstream.is_open());

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

  enableLog("~visusviewer.history.txt");

  clearAll();
  addWorld("world");

  refreshActions();
  setFocusPolicy(Qt::StrongFocus);
  showMaximized();
}

////////////////////////////////////////////////////////////
Viewer::~Viewer()
{
  PrintInfo("destroying VisusViewer");
  RedirectLogTo(nullptr);
  setDataflow(nullptr);
}

////////////////////////////////////////////////////////////
void Viewer::printInfo(String msg)
{
  {
    ScopedLock lock(log.lock);
    log.messages.push_back(msg);
  };

  //I can be here in different thread
  //see //see http://stackoverflow.com/questions/37222069/start-qtimer-from-another-class
  emit postFlushMessages();
}


////////////////////////////////////////////////////////////
void Viewer::execute(Archive& ar)
{
  //action directed to nodes?
  if (GetPassThroughAction("nodes", ar))
  {
    auto uuid = PopTargetId(ar);
    auto node = findNodeByUUID(uuid); VisusAssert(node);
    node->execute(ar);
    return;
  }

  if (ar.name == "Open")
  {
    String parent, url;
    ar.read("parent", parent);
    ar.read("url", url);
    open(url, findNodeByUUID(parent));
    return;
  }

  if (ar.name == "SetAutoRefresh")
  {
    ViewerAutoRefresh value = getAutoRefresh();
    ar.read("enabled", value.enabled);
    ar.read("msec", value.msec);
    setAutoRefresh(value);
    return;
  }

  if (ar.name == "SetMouseDragging")
  {
    bool value;
    ar.read("value", value);
    setMouseDragging(value);
    return;
  }

  if (ar.name == "MoveNode")
  {
    String src, dst; int index;
    ar.read("src", src);
    ar.read("dst", dst);
    ar.read("index", index, -1);
    moveNode(findNodeByUUID(dst), findNodeByUUID(src), index);
    return;
  }

  if (ar.name == "SetSelection")
  {
    String value;
    ar.read("value", value);
    setSelection(findNodeByUUID(value));
    return;
  }

  if (ar.name == "ConnectNodes")
  {
    String from, oport, iport, to;
    ar.read("from", from);
    ar.read("oport", oport);
    ar.read("iport", iport);
    ar.read("to", to);
    connectNodes(findNodeByUUID(from), oport, iport, findNodeByUUID(to));
    return;
  }

  if (ar.name == "DisconnectNodes")
  {
    String from, oport, iport, to;
    ar.read("from", from);
    ar.read("oport", oport);
    ar.read("iport", iport);
    ar.read("to", to);
    disconnectNodes(findNodeByUUID(from), oport, iport, findNodeByUUID(to));
    return;
  }

  if (ar.name == "RefreshNode")
  {
    String node;
    ar.read("node", node);
    refreshNode(findNodeByUUID(node));
    return;
  }

  if (ar.name == "DropProcessing")
  {
    dropProcessing();
    return;
  }

  if (ar.name == "SetNodeVisible") {
    String node; bool value;
    ar.read("node", node);
    ar.read("value", value);
    setNodeVisible(findNodeByUUID(node), value);
    return;
  }

  if (ar.name == "AddNode")
  {
    String parent;  int index;
    ar.read("parent", parent);
    ar.read("index", index, -1);

    auto child = *ar.getFirstChild();
    auto TypeName = child.name;
    auto node = NodeFactory::getSingleton()->createInstance(TypeName); VisusReleaseAssert(node);
    node->read(child);

    addNode(findNodeByUUID(parent), node, index);
    return;
  }

  if (ar.name == "RemoveNode")
  {
    beginTransaction();
    for (auto node : StringUtils::split(ar.readString("uuid")))
      removeNode(findNodeByUUID(node));
    endTransaction();
    return;
  }

  if (ar.name == "AddWorld") {
    String uuid;
    ar.read("uuid", uuid);
    addWorld(uuid);
    return;
  }

  if (ar.name == "AddDataset") {
    String uuid, url, parent;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("url", url);
    addDataset(uuid, findNodeByUUID(parent), url);
    return;
  }

  if (ar.name == "AddGroup") {
    String uuid, name, parent;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("name", name);
    addGroup(uuid, findNodeByUUID(parent), name);
    return;
  }

  if (ar.name == "AddGLCamera") {
    String uuid, type, parent;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("type", type);
    addGLCamera(uuid, findNodeByUUID(parent), type);
    return;
  }

  if (ar.name == "AddSlice")
  {
    String uuid, parent, fieldname; int  access_id;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("fieldname", fieldname);
    ar.read("access_id", access_id, 0);
    addSlice(uuid, findNodeByUUID(parent), fieldname, access_id);
    return;
  }
  if (ar.name == "AddVolume") {
    String uuid, parent, fieldname; int  access_id;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("fieldname", fieldname);
    ar.read("access_id", access_id, 0);
    addVolume(uuid, findNodeByUUID(parent), fieldname, access_id);
    return;
  }

  if (ar.name == "AddIsoContour") {
    String uuid, parent, fieldname; int  access_id; String isovalue;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("fieldname", fieldname);
    ar.read("access_id", access_id, 0);
    ar.read("isovalue", isovalue);
    addIsoContour(uuid, findNodeByUUID(parent), fieldname, access_id, isovalue);
    return;
  }

  if (ar.name == "AddKdQuery") {
    String uuid, parent, fieldname; int  access_id;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("fieldname", fieldname);
    ar.read("access_id", access_id, 0);
    addKdQuery(uuid, findNodeByUUID(parent), fieldname, access_id);
    return;
  }

  if (ar.name == "AddModelView") {
    String uuid, parent;
    bool insert;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("insert", insert, false);
    addModelView(uuid, findNodeByUUID(parent), insert);
    return;
  }

  if (ar.name == "AddScripting") {
    String uuid, parent;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    addScripting(uuid, findNodeByUUID(parent));
    return;
  }

  if (ar.name == "AddPalette") {
    String uuid, parent, palette;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("palette", palette);
    addPalette(uuid, findNodeByUUID(parent), palette);
    return;
  }

  if (ar.name == "AddStatistics") {
    String uuid, parent;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    addStatistics(uuid, findNodeByUUID(parent));
    return;
  }

  if (ar.name == "AddRender") {
    String uuid, parent, palette;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("palette", palette);
    addRender(uuid, findNodeByUUID(parent), palette);
    return;
  }

  if (ar.name == "AddKdRender") {
    String uuid, parent, palette;
    ar.read("uuid", uuid);
    ar.read("parent", parent);
    ar.read("palette", palette);
    addKdRender(uuid, findNodeByUUID(parent), palette);
    return;
  }

  if (ar.name == "TakeSnapshot")
  {
    String type,filename;
    ar.read("type", type);
    ar.read("filename", filename);
    takeSnapshot(/*bOnlyCanvas*/type=="win"?false:true,filename);
    return;
  }

  return Model::execute(ar);
}

///////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<ViewerLogo> Viewer::openScreenLogo(String key, String default_logo)
{
  String filename = config.readString(key + "/filename");
  if (filename.empty())
    filename = default_logo;

  if (filename.empty())
    return SharedPtr<ViewerLogo>();

  auto img = QImage(filename.c_str());
  if (img.isNull()) {
    PrintInfo("Failed to load image",filename);
    return SharedPtr<ViewerLogo>();
  }

  auto tex = GLTexture::createFromQImage(img);
  if (!tex) {
    PrintInfo("Failed to create texture", filename);
    return SharedPtr<ViewerLogo>();
  }

  auto ret = std::make_shared<ViewerLogo>();
  ret->filename = filename;
  ret->tex = tex;
  ret->tex->envmode = GL_MODULATE;
  ret->pos[0] = StringUtils::contains(key, "Left") ? 0 : 1;
  ret->pos[1] = StringUtils::contains(key, "Bottom") ? 0 : 1;
  ret->opacity = cdouble(config.readString(key + "/alpha", "0.5"));
  ret->border = Point2d(10, 10);
  return ret;
};

////////////////////////////////////////////////////////////
void Viewer::setFieldName(String value)
{
  if (auto node = this->findNode<FieldNode>())
    node->setFieldName(value);
}

////////////////////////////////////////////////////////////
void Viewer::setScriptingCode(String value)
{
  if (auto node = this->findNode<ScriptingNode>())
    node->setCode(value);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void Viewer::configureFromArgs(std::vector<String> args)
{
  if (Utils::contains(args,String("--help")))
  {
    PrintInfo("\n",
      "   --visus-config <path>                                                  - path to visus.config","\n",
      "   --open <url>                                                           - opens the specified url or .idx volume", "\n",
      "   --server                                                               - starts a standalone ViSUS Server on port 10000", "\n",
      "   --fullscseen                                                           - starts in fullscreen mode", "\n",
      "   --geometry \"<x> <y> <width> <height>\"                                - specify viewer windows size and location", "\n",
      "   --zoom-to \"x1 y1 x2 y2\"                                              - set glcamera ortho params", "\n",
      "   --network-rcv <port>                                                   - run powerwall slave", "\n",
      "   --network-snd <slave_url> <split_ortho> <screen_bounds> <aspect_ratio> - add a slave to a powerwall master", "\n",
      "   --split-ortho \"x y width height\"                                     - for taking snapshots", "\n",
      "   --internal-network-test-(11|12|14|111)                                 - internal use only", "\n",
      "\n",
      "\n");

    GuiModule::detach();
    exit(0);
  }

  typedef Visus::Rectangle2d Rectangle2d;

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
  String play_file;

  for (int I = 0; I<(int)args.size(); I++)
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
    else if (args[I] == "--play-file")
    {
      play_file = args[++I];
    }

    else if (args[I] == "--server")
    {
      auto modvisus = new ModVisus();
      modvisus->default_public = false;
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
      auto split_ortho = Rectangle2d::fromString(args[++I]);
      auto screen_bounds = Rectangle2d::fromString(args[++I]);
      double fix_aspect_ratio = cdouble(args[++I]);
      this->addNetSnd(url, split_ortho, screen_bounds, fix_aspect_ratio);
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

  if (geometry.width>0 && geometry.height>0)
    this->setGeometry(geometry.x, geometry.y, geometry.width, geometry.height);

  if (!play_file.empty())
    this->playFile(play_file);
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
void Viewer::clearAll()
{
  scheduled.timer.reset();
  setDataflow(std::make_shared<Dataflow>());
  clearHistory();
}

////////////////////////////////////////////////////////////
void Viewer::moveNode(Node* dst, Node* src, int index)
{
  if (!dataflow->canMoveNode(dst, src))
    return;

  beginUpdate(
    StringTree("MoveNode","dst", getUUID(dst)             ,"src", getUUID(src),"index",                   cstring(index)),
    StringTree("MoveNode","dst", getUUID(src->getParent()),"src", getUUID(src),"index", cstring(src->getIndexInParent())));
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
  
  String filename = config.readString("Configuration/VisusViewer/SaveSession/filename", KnownPaths::VisusHome + "/viewer_session.xml");
  
  int every_sec    =cint(config.readString("Configuration/VisusViewer/SaveSession/sec","60")); //1 min!

  //make sure I create a unique filename
  String extension=Path(filename).getExtension();
  if (!extension.empty())
    filename=filename.substr(0,filename.size()-extension.size());  
  filename=filename+"."+Time::now().getFormattedLocalTime()+extension;

  //PrintInfo("Configuration/VisusViewer/SaveSession/filename",filename);
  //PrintInfo("Configuration/VisusViewer/SaveSession/sec",every_sec);

  connect(save_session_timer.get(),&QTimer::timeout,[this,filename](){
    save(filename,/*bSaveHistory*/false);
  });

  if (every_sec>0 && !filename.empty())
    save_session_timer->start(every_sec*1000);
}

//////////////////////////////////////////////////////////////////////
void Viewer::idle()
{
  auto BSize = [](Int64 value) {
    return StringUtils::getStringFromByteSize(value);
  };

  this->dataflow->dispatchPublishedMessages();

  auto io_stats  = File::global_stats();
  auto net_stats = NetService::global_stats();

  auto nthreads     = (Int64)Thread::global_stats()->running_threads;
  auto thread_njobs = (Int64)ThreadPool::global_stats()->running_jobs;
  auto net_njobs    = (Int64)net_stats->tot_requests;

  bool bWasRunning = running.value;
  running.value = thread_njobs > 0 || net_njobs > 0;
  bool bIsRunning = running.value;

  //changle in the status
  if (bIsRunning != bWasRunning)
  {
    if (bIsRunning)
    {
      running.t1 = Time::now();
      io_stats->resetStats();
      net_stats->resetStats();

      running.t2 = Time::now();
    }
    else
    {
      running.elapsed = running.t1.elapsedSec();
    }

    running.io_rbytes = 0;
    running.io_wbytes = 0;

    running.io_rbytes_persec = 0;
    running.io_wbytes_persec = 0;
  }

  auto io_rbytes = (Int64)io_stats->rbytes;
  auto io_wbytes = (Int64)io_stats->wbytes;

  std::ostringstream out;

  if (running.value)
    out << "Running(" << (int)running.t1.elapsedSec() << ") ";
  else
    out << "Done(" << running.elapsed << ") ";


  if (bIsRunning)
  {
    auto t2_elapsed = running.t2.elapsedSec();
    if (t2_elapsed >= 1.0)
    {
      running.io_rbytes_persec = (Int64)((io_rbytes - running.io_rbytes) / t2_elapsed);
      running.io_wbytes_persec = (Int64)((io_wbytes - running.io_wbytes) / t2_elapsed);
      running.t2 = Time::now();
      running.io_rbytes = io_rbytes;
      running.io_wbytes = io_wbytes;
    }
  }

  out << "io_nopen(" << (Int64)io_stats->nopen << ") ";
  out << "io_rb(" << BSize(io_rbytes) << "/" << BSize(running.io_rbytes_persec) << ") ";
  out << "io_wb(" << BSize(io_wbytes) << "/" << BSize(running.io_wbytes_persec) << ") ";

  out << "net_totreq(" << BSize((Int64)net_stats->tot_requests) << ") ";
  out << "net_rb(" << BSize((Int64)net_stats->rbytes) << ") ";
  out << "net_wb(" << BSize((Int64)net_stats->wbytes) << ") "; 

  out << "mem_visus(" << BSize(RamResource::getSingleton()->getVisusUsedMemory()) << ") ";
  out << "mem_used("  << BSize(RamResource::getSingleton()->getOsUsedMemory()) << ") ";
  out << "mem_tot("   << BSize(RamResource::getSingleton()->getOsTotalMemory()) << ") ";

  out << "gpu_tot("  << BSize(GLInfo::getSingleton()->getGpuTotalMemory()) << ") ";
  out << "gpu_used(" << BSize(GLInfo::getSingleton()->getGpuUsedMemory()) << ") ";
  out << "gpu_free(" << BSize(GLInfo::getSingleton()->getGpuFreeMemory()) << ") ";

  out << "nthreads(" << nthreads << ") ";
  out << "thread_njobs(" << thread_njobs << ") ";
  out << "net_njobs(" << net_njobs << ") ";

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
  return computeNodeBounds(getRoot()).toAxisAlignedBox();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Position Viewer::computeNodeBounds(Node* node) const
{
  if (!node)
    node = getRoot();

  //special case for QueryNode: its content is really its bounds
  if (auto query = dynamic_cast<QueryNode*>(node))
    return query->getBounds();

  //NOTE: the result in in local geometric coordinate system of node 
  Position node_bounds = node->getBounds();
  if (node_bounds.valid())
    return node_bounds;

  auto childs = node->getChilds();
  if (childs.empty())
    return Position::invalid();

  //modelview is the only node affecting childs...
  auto T = Matrix::identity(4);
  if (auto modelview_node = dynamic_cast<ModelViewNode*>(node))
    T = modelview_node->getModelView();

  if (childs.size() == 1)
    return Position(T, computeNodeBounds(childs[0]));

  //union of boxes
  auto box = BoxNd::invalid();
  for (auto child : childs)
  {
    Position child_bounds = computeNodeBounds(child);
    if (child_bounds.valid())
      box = box.getUnion(child_bounds.toAxisAlignedBox());
  }
  return Position(T, box);

}



////////////////////////////////////////////////////////////////////////////////////////////////////
Frustum Viewer::computeNodeToScreen(Frustum frustum,Node* node) const
{
  for (auto it : node->getPathFromRoot())
  {
    if (auto modelview_node=dynamic_cast<ModelViewNode*>(it))
    {
      auto T=modelview_node->getModelView();
      frustum.multModelview(T);
    }
  }

  return frustum;
}

//////////////////////////////////////////////////////////////////////
Position Viewer::computeQueryBoundsInDatasetCoordinates(QueryNode* query_node) const
{
  auto dataset_node = query_node->getDatasetNode();

  Position bounds = query_node->getBounds();

  bool bAlreadySimplified = false;
  std::deque<Node*> src2root;
  for (Node* cursor = query_node; cursor; cursor = cursor->getParent())
  {
    if (cursor == dataset_node)
    {
      bAlreadySimplified = true;
      break;
    }
    src2root.push_back(cursor);
  }

  std::deque<Node*> root2dst;
  if (!bAlreadySimplified)
  {
    for (Node* cursor = dataset_node; cursor; cursor = cursor->getParent())
      root2dst.push_front(cursor);

    //symbolic simplification (i.e. if I traverse the same node back and forth)
    while (!src2root.empty() && !root2dst.empty() && src2root.back() == root2dst.front())
    {
      src2root.pop_back();
      root2dst.pop_front();
    }
  }

  for (auto it = src2root.begin(); it != src2root.end(); it++)
  {
    if (auto modelview_node = dynamic_cast<ModelViewNode*>(*it))
      bounds = Position(modelview_node->getModelView(), bounds);
  }

  for (auto it = root2dst.begin(); it != root2dst.end(); it++)
  {
    if (auto modelview_node = dynamic_cast<ModelViewNode*>(*it))
      bounds = Position(modelview_node->getModelView().invert(), bounds);
  }

  return bounds;
}

////////////////////////////////////////////////////////////
Node* Viewer::findPick(Node* node,Point2d screen_point,bool bRecursive,double* out_distance) const
{
  if (!node)
    return nullptr;

  Node*  ret=nullptr;
  double best_distance=NumericLimits<double>::highest();
  auto viewport = widgets.glcanvas->getViewport();
    
  //I allow the picking of only queries
  if (auto query=dynamic_cast<QueryNode*>(node))
  {
    Frustum  node_to_screen = computeNodeToScreen(getGLCamera()->getCurrentFrustum(viewport),node);
    Position node_bounds  = node->getBounds();

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
  Position bounds=query_node->getBounds();
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
    free_transform->object_changed.connect([this, query_node](Position query_bounds)
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
      query_node->setBounds(query_bounds);
      free_transform->setObject(query_bounds);
    });

  }

  free_transform->setObject(bounds);
  postRedisplay();
}

///////////////////////////////////////////////////////////////////////////////
void Viewer::beginFreeTransform(ModelViewNode* modelview_node)
{
  //this is what I want to edit
  auto T = modelview_node->getModelView();

  //take off the effect of T
  auto bounds = computeNodeBounds(modelview_node);
  bounds = Position(T.invert(), bounds);

  if (!T.valid() || !bounds.valid()) 
  {
    free_transform.reset();
    postRedisplay();
    return;
  }

  if (!free_transform)
  {
    free_transform=std::make_shared<FreeTransform>();

    free_transform->object_changed.connect([modelview_node,bounds](Position obj)
    {
      auto T=obj.getTransformation() * bounds.getTransformation().invert();
      modelview_node->setModelView(T);
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
    auto query_bounds= computeQueryBoundsInDatasetCoordinates(query_node);
    query_node->setQueryBounds(query_bounds);

    //overwrite the viewdep frustum, since QueryNode works in dataset reference space
    //NOTE: using the FINAL frusutm
    auto viewport = widgets.glcanvas->getViewport();
    auto node_to_screen=computeNodeToScreen(getGLCamera()->getFinalFrustum(viewport),query_node->getDatasetNode());
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
      removeDockWidget(dock_widget);
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

    if (preferences.screen_bounds.valid())
      setGeometry(QUtils::convert<QRect>(preferences.screen_bounds));

    //titlebar
    setWindowTitle(preferences.title.c_str());

    //toolbar
    if (preferences.bShowToolbar)
      widgets.toolbar->show();
    else
      widgets.toolbar->hide();


    if (preferences.bShowTreeView)
    {
      widgets.treeview = createTreeView();
      auto dock = new QDockWidget("Explorer");
      dock->setWidget(widgets.treeview);
      addDockWidget(Qt::LeftDockWidgetArea, dock);
    }

    //glcanvas
    widgets.glcanvas = createGLCanvas();

    if (preferences.bShowDataflow)
    {
      widgets.frameview = new DataflowFrameView(this->dataflow.get());

      widgets.tabs = new QTabWidget();
      widgets.tabs->addTab(widgets.glcanvas, "GLCanvas");
      widgets.tabs->addTab(widgets.frameview, "Dataflow");
      setCentralWidget(widgets.tabs);
    }
    else
    {
      setCentralWidget(widgets.glcanvas);
    }

    //logs
    if (preferences.bShowLogs)
    {
      widgets.log = GuiFactory::CreateTextEdit(Colors::Black, Color(230, 230, 230));

      auto dock = new QDockWidget("Log");
      dock->setWidget(widgets.log);
      addDockWidget(Qt::BottomDockWidgetArea, dock);
    }

    if (auto glcamera_node = findNode<GLCameraNode>())
      attachGLCamera(glcamera_node->getGLCamera());

    enableSaveSession();

    //timer
    this->idle_timer.reset(new QTimer());
    connect(this->idle_timer.get(), &QTimer::timeout, [this]() {
      idle();
    });
    const int fps = 20;
    this->idle_timer->start(1000/ fps); 

    this->refreshNode();
    this->postRedisplay();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Viewer::reloadVisusConfig(bool bChooseAFile)
{
  if (bChooseAFile)
  {
    static String last_dir(KnownPaths::VisusHome);
    String filename = cstring(QFileDialog::getOpenFileName(nullptr, "Choose a file to open...", last_dir.c_str(), "*"));
    if (filename.empty()) return;
    last_dir = Path(filename).getParent();
    this->config.load(filename);
  }
  else
  {
    this->config.reload();
  }

  this->widgets.toolbar->bookmarks_button->setMenu(createBookmarks());
}
 
//////////////////////////////////////////////////////////////////////
bool Viewer::open(String s_url,Node* parent)
{
  if (s_url.empty())
    return false;

  //get a visus.config from a remote server
  //  http://atlantis.sci.utah.edu:8080/mod_visus?action=list
  //  http://atlantis.sci.utah.edu:8080/mod_visus?action=list&cached=1
  Url url(s_url);
  if (url.isRemote() && url.getPath()=="/mod_visus" && url.getParam("action")=="list")
  {
    bool cached = cbool(url.getParam("cached", "false"));
    url.params.eraseValue("cached");

    auto content=Utils::loadTextDocument(url.toString());
    content = StringUtils::replaceAll(content, "$(protocol)", url.getProtocol());
    content = StringUtils::replaceAll(content, "$(hostname)", url.getHostname());
    content = StringUtils::replaceAll(content, "$(port)", cstring(url.getPort()));

    auto config = StringTree::fromString(content);
    if (!config.valid())
    {
      QMessageBox::information(this, "Error", cstring("Failed to download",url.toString()).c_str());
      return false;
    }

    //handle caching...
    if (cached)
    {
      for (auto dataset : config.getAllChilds("dataset"))
      {
        if (!dataset->getChild("access") && !dataset->getAttribute("name").empty())
        {
          auto dataset_name = dataset->getAttribute("name");
          dataset->addChild(StringTree::fromString(
            "  <access type='multiplex'>\n"
            "     <access type='disk'    chmod='rw' url='file://$(VisusHome)/cache/" + url.getHostname() + "/" + cstring(url.getPort()) + "/" + dataset_name + "/visus.idx'  />\n"
            "     <access type='network' chmod='r'  compression='zip' />\n"
            "  </access>\n"));
        }
      }
    }

    this->config = config;
    Utils::saveTextDocument("temp.config", this->config.toString());
    reloadVisusConfig();
    QMessageBox::information(this, "Info", cstring("Loaded", url.toString(), "saved the content to temp.config").c_str());
    return true;
  }

  //open a *.config file and create bookmarks
  if (StringUtils::endsWith(s_url,".config"))
  {
    auto content = Utils::loadTextDocument(s_url);
    auto config=StringTree::fromString(content);
    if (!config.valid())
    {
      VisusAssert(false);
      return false;
    }

    this->config = config;
    reloadVisusConfig();
    return true;
  }

  //could be a Viewer scene?
  if (StringUtils::endsWith(s_url,".xml"))
  {
    auto content = StringUtils::trim(Utils::loadTextDocument(s_url));
    if (StringUtils::startsWith(content, "<Viewer"))
    {
      clearAll();
      setDataflow(std::make_shared<Dataflow>());

      try
      {
        auto ar = StringTree::fromString(content);
        read(ar);
      }
      catch (const std::exception& ex)
      {
        VisusAssert(false);
        QMessageBox::information(this, "Error", ex.what());
        return false;
      }

      PrintInfo("worldbox",getWorldBox().toString());

      PrintInfo("open", s_url, "done");

      if (widgets.treeview)
        widgets.treeview->expandAll();
      refreshActions();
      return true;
    }
  }

  //open a dataset
  SharedPtr<Dataset> dataset;
  try
  {
    dataset = LoadDatasetEx(FindDatasetConfig(this->config, s_url));
  } 
  catch(...)
  {
    QMessageBox::information(this, "Error", cstring("open file",concatenate("[", s_url, "]"),"failed.").c_str());
    return false;
  }

  //do I need to add a glcamera too?
  if (!parent) 
    clearAll();

  beginTransaction();
  {
    if (!parent)
      parent = addWorld("world");

    auto dataset_node=addDataset("",parent, s_url);

    if (!getGLCamera())
      addGLCamera("", parent);

    //add a default render node
    if (bool bAddRenderNode=true)
    {
      String rendertype = StringUtils::toLower(dataset->getDatasetBody().readString("rendertype", ""));

      if ((dataset->getKdQueryMode() != KdQueryMode::NotSpecified) || rendertype == "kdrender")
        addKdQuery("", dataset_node);

      else if (dataset->getPointDim() == 3)
        addVolume("", dataset_node);

      else
        addSlice("", dataset_node);
    }

    refreshAll();

  }
  endTransaction();

  if (widgets.treeview)
    widgets.treeview->expandAll();
  
  refreshActions();
  PrintInfo("open",s_url,"done");
  return true;
}

//////////////////////////////////////////////////////////////////////
bool Viewer::playFile(String url)
{
  if (url.empty())
  {
    url = cstring(QFileDialog::getOpenFileName(nullptr, "Choose a file to open...", last_filename.c_str(),"XML files (*.xml)"));
    if (url.empty()) return false;
    this->last_filename = url;
  }

  auto ar = StringTree::fromString(Utils::loadTextDocument(url));
  if (!ar.valid())
  {
    VisusAssert(false);
    return false;
  }

  double version;
  ar.read("version", version, 0.0);

  String git_revision;
  ar.read("git_revision", git_revision);

  clearAll();
  Int64 first_utc=0;
  scheduled.timer = std::make_shared<QTimer>();
  QObject::connect(scheduled.timer.get(), &QTimer::timeout, [this]() {

    scheduled.timer->stop();

    //finished
    if (scheduled.actions.empty())
      return;

    auto first = scheduled.actions.front();

    scheduled.actions.pop_front();
    if (!scheduled.actions.empty())
    {
      auto next = scheduled.actions.front();
      Int64 t1, t2;
      first.read("utc", t1);
      next.read("utc", t2);
      auto delta = std::max(t2 - t1,(Int64)0);
      scheduled.timer->start(delta);
    }

    execute(first);
  });

  for (auto action : ar.getChilds())
  {
    if (action->isHash()) continue;
    scheduled.actions.push_back(*action);
  }
  scheduled.timer->start(0);
  
  return true;
}

//////////////////////////////////////////////////////////////////////
bool Viewer::openFile(String url, Node* parent)
{
  if (url.empty())
  {
    url = cstring(QFileDialog::getOpenFileName(nullptr, "Choose a file to open...", last_filename.c_str(),
      "All supported (*.idx *.midx *.gidx *.obj *.xml *.config *.scn);;IDX (*.idx *.midx *.gidx);;OBJ (*.obj);;XML files (*.xml *.config *.scn)"));

    if (url.empty()) return false;
    last_filename = url;
    url = StringUtils::replaceAll(url, "\\", "/");
    if (!StringUtils::startsWith(url, "/")) url = "/" + url;
    url = "file://" + url;
  }

  return open(url, parent);
}

//////////////////////////////////////////////////////////////////////
bool Viewer::openUrl(String url, Node* parent)
{
  if (url.empty())
  {
    static String last_url("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1");
    url = cstring(QInputDialog::getText(this, "Enter the url:", "", QLineEdit::Normal, last_url.c_str()));
    if (url.empty()) return false;
    last_url = url;
  }

  return open(url, parent);
}

//////////////////////////////////////////////////////////////////////
void Viewer::save(String url,bool bSaveHistory)
{ 
  if (url.empty())
    ThrowException("invalid url");

  //add default extension
  if (Path(url).getExtension().empty())
    url=url+".xml";

  StringTree ar;
  if (bSaveHistory)
  {
    ar = getHistory();
    ar.name = "Viewer";
    ar.write("version", OpenVisus_VERSION);
    ar.write("git_revision", OpenVisus_GIT_REVISION);
  }
  else
  {
    ar =StringTree("Viewer");
    this->write(ar);
  }

  Utils::saveTextDocument(url, ar.toString());
  this->last_saved_filename=url;
}

////////////////////////////////////////////////////////////
void Viewer::saveFile(String url, bool bSaveHistory)
{
  if (url.empty())
  {
    static String last_dir(KnownPaths::VisusHome);
    url = cstring(QFileDialog::getSaveFileName(nullptr, "Choose a file to save...", last_dir.c_str(), "*.xml"));
    if (url.empty()) return ;
    last_dir = Path(url).getParent();
  }

  try
  {
    save(url, bSaveHistory);
  }
  catch (...)
  {
    QMessageBox::information(this, "Error", cstring("Failed to save file", url).c_str());
    return;
  }
}
  
////////////////////////////////////////////////////////////
void Viewer::setAutoRefresh(ViewerAutoRefresh new_value)  {

  auto& old_value = this->auto_refresh;

  //useless call
  if (old_value.msec == new_value.msec && old_value.enabled == new_value.enabled)
    return;

  beginUpdate(
    StringTree("SetAutoRefresh","enabled", new_value.enabled,"msec", new_value.msec),
    StringTree("SetAutoRefresh","enabled", old_value.enabled,"msec", old_value.msec));
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
      refreshAll();
    });
    this->auto_refresh_timer->start(this->auto_refresh.msec);
  }

}

////////////////////////////////////////////////////////////////////
void Viewer::dropProcessing()
{
  beginUpdate(
    StringTree("DropProcessing"),
    StringTree("DropProcessing"));
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
  auto& old_value = this->mouse_dragging;
  if (old_value == new_value) return;
  beginUpdate(
    StringTree("SetMouseDragging", "value", new_value),
    StringTree("SetMouseDragging", "value", old_value));
  {
    old_value = new_value;
  }
  endUpdate();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::scheduleMouseDragging(bool value, int msec)
{
  //stop dragging: postpone a little the end-drag event for the camera
  this->mouse_timer.reset(new QTimer());
  connect(this->mouse_timer.get(), &QTimer::timeout, [this, value] {
    this->mouse_timer.reset();
    setMouseDragging(value);
  });
  this->mouse_timer->start(msec);
}

////////////////////////////////////////////////////////////////////
void Viewer::setSelection(Node* new_value)
{
  auto old_value=getSelection();
  if (old_value == new_value)
    return;

  beginUpdate(
    StringTree("SetSelection", "value", getUUID(new_value)),
    StringTree("SetSelection", "value", getUUID(old_value)));
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
void Viewer::setMinimal()
{
  ViewerPreferences pref;
  pref.bShowTitleBar = true;
  pref.bShowToolbar = false;
  pref.bShowTreeView = false;
  pref.bShowDataflow = false;
  pref.bShowLogs = false;
  pref.bShowLogos = true;
  this->setPreferences(pref);
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
    StringTree("SetNodeVisible", "node", getUUID(node), "value", new_value),
    StringTree("SetNodeVisible", "node", getUUID(node), "value", old_value));
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

  node->begin_update.connect([this](){
    beginTransaction();
  });

  node->end_update.connect([this,node]()
  {
    beginUpdate(
      CreatePassThroughAction(concatenate("nodes", "/", getUUID(node)), node->lastRedo()), 
      CreatePassThroughAction(concatenate("nodes", "/", getUUID(node)), node->lastUndo()));
    {
      //if something changes in the query...
      if (auto query_node = dynamic_cast<QueryNode*>(node))
        refreshNode(query_node);

      else if (auto modelview_node = dynamic_cast<ModelViewNode*>(node))
        refreshNode(modelview_node);
    }
    endUpdate();

    endTransaction();
  });

  dropSelection();

  beginTransaction();
  {
    StringTree encoded(node->getTypeName());
    node->write(encoded);

    beginUpdate(
      StringTree("AddNode", "parent", getUUID(parent), "index",index).addChild(encoded),
      StringTree("RemoveNode", "uuid",getUUID(node)));
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

  //all uno actions inside beginUpdate/endUpdate will be appended to the Transaction
  auto undo = Transaction();

  beginUpdate(
    StringTree("RemoveNode", "uuid",getUUID(NODE)),
    undo);
  {
    dropProcessing();
    dropSelection();

    auto rev = NODE->reversedBreadthFirstSearch();

    //first disconnect all ports
    for (auto node : rev)
    {

      if (auto glcamera_node = dynamic_cast<GLCameraNode*>(node))
        detachGLCamera();

      for (auto input : node->inputs)
      {
        DataflowPort* iport = input.second; VisusAssert(iport->outputs.empty());//todo multi dataflow
        while (!iport->inputs.empty())
        {
          auto oport = *iport->inputs.begin();
          disconnectNodes(oport->getNode(), oport->getName(), iport->getName(), iport->getNode());
        }
      }

      for (auto output : node->outputs)
      {
        auto oport = output.second; VisusAssert(oport->inputs.empty());//todo multi dataflow
        while (!oport->outputs.empty())
        {
          auto iport = *oport->outputs.begin();
          disconnectNodes(oport->getNode(), oport->getName(), iport->getName(), iport->getNode());
        }
      }
    }

    //then remove the nodes
    for (auto node : rev)
    {
      VisusAssert(node->getChilds().empty()); 
      VisusAssert(node->isOrphan());

      beginUpdate(
        StringTree(),
        StringTree(StringTree("AddNode")
          .write("parent", getUUID(node->getParent()))
          .write("index", cstring(node->getIndexInParent()))
          .addChild(EncodeObject(node->getTypeName(), *this))));
      {
        dataflow->removeNode(node);
      }
      endUpdate();
    }

    autoConnectNodes();
  }
  endUpdate();

  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::connectNodes(Node* from,String oport,String iport,Node* to)
{
  beginUpdate(
    StringTree("ConnectNodes",    "from", getUUID(from), "oport", oport, "iport", iport, "to", getUUID(to)),
    StringTree("DisconnectNodes", "from", getUUID(from), "oport", oport, "iport", iport, "to", getUUID(to)));
  {
    dataflow->connectNodes(from, oport, iport, to);
  }
  endUpdate();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////
void Viewer::connectNodes(Node* from, String port, Node* to) {
  VisusAssert(from->hasOutputPort(port));
  VisusAssert(to->hasInputPort(port));
  connectNodes(from, port, port, to);
}

////////////////////////////////////////////////////////////////////
void Viewer::connectNodes(Node* from, Node* to)
{
  std::vector<String> common;
  for (auto oport : from->getOutputPortNames())
  {
    if (to->hasInputPort(oport))
      common.push_back(oport);
  }
  if (common.size() != 1)
    ThrowException("internal error");
  return connectNodes(from, common[0], to);
}

////////////////////////////////////////////////////////////////////
void Viewer::disconnectNodes(Node* from,String oport,String iport,Node* to)
{
  beginUpdate(
    StringTree("DisconnectNodes", "from", getUUID(from), "oport", oport, "iport", iport, "to", getUUID(to)),
    StringTree("ConnectNodes"   , "from", getUUID(from), "oport", oport, "iport", iport, "to", getUUID(to)));
  {
    dataflow->disconnectNodes(from, oport, iport, to);
  }
  endUpdate();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////////
void Viewer::autoConnectNodes()
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

      //I have a child with no input ports and one output port
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
        connectNodes(oport->getNode(), oport->getName(), iport->getName(), iport->getNode());
    }
  }
  endTransaction();

  postRedisplay();
}

/////////////////////////////////////////////////////////////////
void Viewer::refreshNode(Node* node)
{
  beginUpdate(
    StringTree("RefreshNode").writeIfNotDefault("node", getUUID(node), String("")),
    StringTree("RefreshNode").writeIfNotDefault("node", getUUID(node), String("")));

  if (node)
  {
    if (auto query_node=dynamic_cast<QueryNode*>(node))
    {
      dataflow->needProcessInput(query_node);
    }
    else if (auto modelview_node=dynamic_cast<ModelViewNode*>(node))
    {
      //find all queries, they could have been their relative position in the dataset changed
      for (auto it : modelview_node->breadthFirstSearch())
      {
        if (auto query_node=dynamic_cast<QueryNode*>(it))
        {
          Position query_bounds= computeQueryBoundsInDatasetCoordinates(query_node);
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
Node* Viewer::addWorld(String uuid)
{
  if (uuid.empty())
    uuid = "world";

  Node* ret = nullptr;
  beginUpdate(
    StringTree("AddWorld", "uuid", uuid),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //world
    auto world = new Node();
    ret = world;
    world->setUUID(uuid);
    world->setName("World");
    addNode(world);
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DatasetNode* Viewer::addDataset(String uuid, Node* parent, String url)
{
  if (!parent)
    parent = getRoot();

  auto dataset = LoadDatasetEx(FindDatasetConfig(this->config, url));

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("dataset");

  dropSelection();

  DatasetNode* ret = nullptr;
  beginUpdate(
    StringTree("AddDataset", "uuid", uuid, "parent", getUUID(parent), "url", url),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //dataset
    auto dataset_node = new DatasetNode();
    ret = dataset_node;
    dataset_node->setUUID(uuid);
    dataset_node->setName(url);
    dataset_node->setDataset(dataset);
    dataset_node->setShowBounds(true);
    addNode(parent, dataset_node);

    //time (this is for queries...)
    auto time_node = new TimeNode(dataset->getTime(), dataset->getTimesteps());
    time_node->setUUID(uuid, "time");
    time_node->setName("Time");
    addNode(dataset_node, time_node);
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ModelViewNode* Viewer::addModelView(String uuid, Node* parent, bool insert)
{
  if (!parent)
    parent = getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("modelview");

  StringTree undo;
  if (insert)
    undo = Transaction(); //every undo actions will be appended to Transaction
  else
    undo = StringTree("RemoveNode", "uuid", uuid);

  ModelViewNode* ret = nullptr;
  beginUpdate(
    StringTree("AddModelView", "uuid", uuid, "parent", getUUID(parent), "insert", insert),
    undo);
  {
    //modelview
    auto modelview_node = new ModelViewNode();
    ret = modelview_node;
    modelview_node->setUUID(uuid);
    modelview_node->setName("ModelView");
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

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addGroup(String uuid, Node* parent, String name)
{
  if (!parent)
    parent = getRoot();

  if (name.empty())
  {
    name = cstring(QInputDialog::getText(this, "Insert the group name:", "", QLineEdit::Normal, ""));
    if (name.empty())
      return nullptr;
  }

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("group");

  dropSelection();

  Node* ret = nullptr;
  beginUpdate(
    StringTree("AddGroup", "uuid", uuid, "parent", getUUID(parent), "name",name),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //group
    auto group_node = new Node();
    ret = group_node;
    group_node->setUUID(uuid);
    group_node->setName(name);
    addNode(parent, group_node, /*index*/0);
  }
  endUpdate();

  return ret;
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
GLCameraNode* Viewer::addGLCamera(String uuid, Node* parent, String type)
{
  if (!parent)
    parent=getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("glcamera");

  type = StringUtils::toLower(type);
  if (type.empty())
    type = getWorldBox().toBox3().minsize() == 0? "ortho" : "lookat";

  dropSelection();

  GLCameraNode* ret = nullptr;
  beginUpdate(
    StringTree("AddGLCamera", "uuid", uuid, "parent", getUUID(parent), "type", type),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //glcamera
    SharedPtr<GLCamera> glcamera;
    if (StringUtils::contains(type, "ortho"))
      glcamera = std::make_shared<GLOrthoCamera>();
    else
      glcamera = std::make_shared<GLLookAtCamera>();
    glcamera->guessPosition(getWorldBox());

    auto glcamera_node = new GLCameraNode(glcamera);
    ret = glcamera_node;
    glcamera_node->setUUID(uuid);
    glcamera_node->setName("GLCamera");
    addNode(parent, glcamera_node,/*index*/0);
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addVolume(String uuid, Node* parent, String fieldname, int access_id)
{
  if (!parent)
  {
    parent = findNode<DatasetNode>();
    if (!parent)
      parent = getRoot();
  }

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("volume");

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNode<DatasetNode>();
  VisusReleaseAssert(dataset_node);
  auto dataset = dataset_node->getDataset();

  if (fieldname.empty())
    fieldname = dataset->getField().name;

  QueryNode* ret = nullptr;
  beginUpdate(
    StringTree("AddVolume", "uuid", uuid, "parent", getUUID(parent), "fieldname", fieldname, "access_id", access_id),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //query
    auto query_node = new QueryNode();
    ret = query_node;
    query_node->setUUID(uuid);
    query_node->setName("Volume");
    query_node->setVerbose(1);
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setProgression(QueryGuessProgression);
    query_node->setQuality(QueryDefaultQuality);
    query_node->setBounds(dataset_node->getBounds());

    addNode(parent, query_node);
    connectNodes(dataset_node, query_node);

    //time
    auto time_node = dataset_node->findChild<TimeNode*>();
    if (!time_node) {
      time_node = new TimeNode(dataset->getTime(), dataset->getTimesteps());
      time_node->setUUID(uuid, "time");
      time_node->setName("Time");
      addNode(query_node, time_node);
    }
    connectNodes(time_node, query_node);

    //field
    auto field_node = new FieldNode();
    field_node->setUUID(uuid, "field");
    field_node->setName("Field");
    field_node->setFieldName(fieldname);
    addNode(query_node, field_node);
    connectNodes(field_node, query_node);

    //scripting
    auto scripting_node = NodeFactory::getSingleton()->createInstance("ScriptingNode");
    scripting_node->setUUID(uuid, "scripting");
    scripting_node->setName("Scripting");
    addNode(query_node, scripting_node);
    connectNodes(query_node, scripting_node);

    //render
    addRender(concatenate(uuid, "_render"), scripting_node, "GrayTransparent");
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addSlice(String uuid, Node* parent, String fieldname, int access_id)
{
  if (!parent)
  {
    parent = findNode<DatasetNode>();
    if (!parent)
      parent = getRoot();
  }

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("slice");

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNode<DatasetNode>();
  VisusReleaseAssert(dataset_node);
  auto dataset = dataset_node->getDataset();

  if (fieldname.empty())
    fieldname = dataset->getField().name;

  QueryNode* ret = nullptr;
  beginUpdate(
    StringTree("AddSlice", "uuid", uuid, "parent", getUUID(parent), "fieldname", fieldname, "access_id", access_id),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //query
    auto query_node = new QueryNode();
    ret = query_node;
    query_node->setUUID(uuid);
    query_node->setName("Slice");
    query_node->setVerbose(1);
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setProgression(QueryGuessProgression);
    query_node->setQuality(QueryDefaultQuality);
    if (bool bPointQuery = dataset->getPointDim() == 3)
    {
      auto box = dataset_node->getBounds().getBoxNd().withPointDim(3);
      box.p1[2] = box.p2[2] = box.center()[2];
      query_node->setBounds(Position(dataset_node->getBounds().getTransformation(), box));
    }
    else
    {
      query_node->setBounds(dataset_node->getBounds());
    }
    addNode(parent, query_node);
    connectNodes(dataset_node, query_node);

    //time
    auto time_node = dataset_node->findChild<TimeNode*>();
    if (!time_node)
    {
      time_node = new TimeNode(dataset->getTime(), dataset->getTimesteps());
      time_node->setUUID(uuid, "time");
      time_node->setName("Time");
      addNode(query_node, time_node);
    }
    connectNodes(time_node, query_node);

    //field
    auto field_node = new FieldNode();
    field_node->setUUID(uuid, "field");
    field_node->setName("Field");
    field_node->setFieldName(fieldname);
    addNode(query_node, field_node);
    connectNodes(field_node, query_node);

    //scripting
    auto scripting_node = NodeFactory::getSingleton()->createInstance("ScriptingNode");
    scripting_node->setUUID(uuid, "scripting");
    scripting_node->setName("Scripting");
    addNode(query_node, scripting_node);
    connectNodes(query_node, scripting_node);

    //render
    addRender(concatenate(uuid, "_render"), scripting_node, "GrayOpaque");
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addIsoContour(String uuid, Node* parent, String fieldname, int access_id, String s_isovalue)
{
  if (!parent)
  {
    parent = findNode<DatasetNode>();
    if (!parent)
      parent = getRoot();
  }

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("isocontour");

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNode<DatasetNode>();
  VisusReleaseAssert(dataset_node);
  auto dataset = dataset_node->getDataset();
  VisusReleaseAssert(dataset);

  if (fieldname.empty())
    fieldname = dataset->getField().name;

  dropSelection();

  QueryNode* ret = nullptr;
  beginUpdate(
    StringTree("AddIsoContour", "uuid", uuid, "parent", getUUID(parent), "fieldname", fieldname, "access_id", access_id, "isovalue", s_isovalue),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //query
    auto query_node = new QueryNode();
    ret = query_node;
    query_node->setUUID(uuid);
    query_node->setName("IsoContour");
    query_node->setVerbose(1);
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setProgression(QueryGuessProgression);
    query_node->setQuality(QueryDefaultQuality);
    query_node->setBounds(dataset_node->getBounds());
    addNode(parent, query_node);
    connectNodes(dataset_node, query_node);

    //time
    auto time_node = dataset_node->findChild<TimeNode*>();
    if (!time_node) {
      time_node = new TimeNode(dataset->getTime(), dataset->getTimesteps());
      time_node->setUUID(uuid, "time");
      time_node->setName("Time");
      addNode(query_node, time_node);
    }
    connectNodes(time_node, query_node);

    //field
    auto field_node = new FieldNode();
    field_node->setUUID(uuid, "field");
    field_node->setName("Field");
    field_node->setFieldName(fieldname);
    addNode(query_node, field_node);
    connectNodes(field_node, query_node);

    //scripting
    auto scripting_node = NodeFactory::getSingleton()->createInstance("ScriptingNode");
    scripting_node->setUUID(uuid, "scripting");
    scripting_node->setName("Scripting");
    addNode(query_node, scripting_node);
    connectNodes(query_node, scripting_node);

    //build isocontour
    auto build_isocontour = new IsoContourNode();
    build_isocontour->setUUID(uuid, "isocontour");
    build_isocontour->setName("IsoContour");
    double isovalue = 0.0;
    if (!s_isovalue.empty()) {
      isovalue = cdouble(s_isovalue);
    }
    else {
      Field field = dataset->getField(fieldname);
      isovalue = field.valid() && field.dtype.isVectorOf(DTypes::UINT8) ? 128.0 : 0.0;
    }
    build_isocontour->setIsoValue(isovalue);
    addNode(scripting_node, build_isocontour);
    connectNodes(scripting_node, build_isocontour);

    //palette
    //this is useful if the data coming from the data provider has 2 components, first is used to compute the 
    //marching cube, the second as a second field to color the vertices of the marching cube
    auto palette_node = new PaletteNode("GrayOpaque");
    palette_node->setUUID(uuid, "palette");
    palette_node->setName("Palette");
    addNode(scripting_node, palette_node);
    connectNodes(scripting_node, palette_node); //this is for statistics

    //render
    auto render_node = new IsoContourRenderNode();
    render_node->setUUID(uuid, "render");
    render_node->setName("MeshRender");
    GLMaterial material = render_node->getMaterial();
    material.front.diffuse = Color::createFromUint32(0x3c6d3eff);
    material.front.specular = Color::createFromUint32(0xffffffff);
    material.back.diffuse = Color::createFromUint32(0x2a4b70ff);
    material.back.specular = Color::createFromUint32(0xffffffff);
    render_node->setMaterial(material);
    addNode(scripting_node, render_node);
    connectNodes(build_isocontour, render_node);
    connectNodes(palette_node, render_node);
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
KdQueryNode* Viewer::addKdQuery(String uuid, Node* parent,String fieldname,int access_id)
{
  if (!parent)
  {
    parent = findNode<DatasetNode>();
    if (!parent)
      parent = getRoot();
  }

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("kdquery");

  auto* dataset_node = dynamic_cast<DatasetNode*>(parent);
  if (!dataset_node)
    dataset_node = findNode<DatasetNode>();
  VisusReleaseAssert(dataset_node);
  auto dataset = dataset_node->getDataset();

  if (fieldname.empty())
    fieldname = dataset->getField().name;

  KdQueryNode* ret = nullptr;
  beginUpdate(
    StringTree("AddKdQuery", "uuid", uuid, "parent", getUUID(parent), "fieldname", fieldname, "access_id", access_id),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //query
    auto query_node = new KdQueryNode();
    ret = query_node;
    query_node->setUUID(uuid);
    query_node->setName("KdQuery");
    query_node->setAccessIndex(access_id);
    query_node->setViewDependentEnabled(true);
    query_node->setQuality(QueryDefaultQuality);
    query_node->setBounds(dataset_node->getBounds());
    addNode(parent, query_node);
    connectNodes(dataset_node, query_node);

    //time
    auto time_node = dataset_node->findChild<TimeNode*>();
    if (!time_node) {
      time_node = new TimeNode(dataset->getTime(), dataset->getTimesteps());
      time_node->setUUID(uuid, "time");
      time_node->setName("Time");
      addNode(query_node, time_node);
    }
    connectNodes(time_node, query_node);

    //field
    auto field_node = new FieldNode();
    field_node->setUUID(uuid, "field");
    field_node->setName("Field");
    field_node->setFieldName(fieldname);
    addNode(query_node, field_node);
    connectNodes(field_node, query_node);

    addKdRender(concatenate(uuid, "_render"), query_node, "GrayOpaque");
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ScriptingNode* Viewer::addScripting(String uuid, Node* parent)
{
  if (!parent)
    parent = getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("scripting");

  dropSelection();

  ScriptingNode* ret = nullptr;
  beginUpdate(
    StringTree("AddScripting", "uuid", uuid, "parent", getUUID(parent)),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //scripting
    auto scripting_node = NodeFactory::getSingleton()->createInstance("ScriptingNode");
    scripting_node->setUUID(uuid);
    scripting_node->setName("Scripting");
    addNode(parent, scripting_node);
    connectNodes(parent, scripting_node);

    //render
    addRender(concatenate(uuid, "_render"), scripting_node, "GrayOpaque");
  }
  endUpdate();

  return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
StatisticsNode* Viewer::addStatistics(String uuid, Node* parent)
{
  if (!parent)
    parent = getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("statistics");

  dropSelection();

  StatisticsNode* ret = nullptr;
  beginUpdate(
    StringTree("AddStatistics", "uuid", uuid, "parent", getUUID(parent)),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //statistics
    auto statistics_node = new StatisticsNode();
    ret = statistics_node;
    statistics_node->setUUID(uuid);
    statistics_node->setName("Statistics");
    addNode(parent, statistics_node);
    connectNodes(parent, statistics_node);
  }
  endUpdate();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
PaletteNode* Viewer::addPalette(String uuid, Node* parent,String palette)
{
  if (!parent)
    parent = getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("palette");

  dropSelection();

  PaletteNode* ret = nullptr;
  beginUpdate(
    StringTree("AddPalette", "uuid", uuid, "parent", getUUID(parent), "palette", palette),
    StringTree("RemoveNode", "uuid", uuid));
  {
    auto palette_node = new PaletteNode(palette);
    ret = palette_node;
    palette_node->setUUID(uuid);
    palette_node->setName("Palette");
    addNode(parent, palette_node, 1);

    //enable statistics
    if (parent->hasOutputPort("array"))
      connectNodes(parent, palette_node);
  }
  endUpdate();

  return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addRender(String uuid, Node* parent, String palette)
{
  if (!parent)
    parent = getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("render");

  dropSelection();

  Node* ret = nullptr;
  beginUpdate(
    StringTree("AddRender", "uuid", uuid, "parent", getUUID(parent), "palette", palette),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //render
    Node *render_node = new RenderArrayNode();
    ret = render_node;
    render_node->setUUID(uuid);
    render_node->setName("RenderArray");
    addNode(parent, render_node);
    connectNodes(parent, render_node);

    //palette
    if (!palette.empty())
    {
      auto palette_node = new PaletteNode(palette);
      palette_node->setUUID(uuid, "palette");
      palette_node->setName("Palette");
      addNode(render_node, palette_node);
      connectNodes(parent, palette_node); //this is for statistics
      connectNodes(palette_node, render_node);
    }
  }
  endUpdate();

  return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
KdRenderArrayNode* Viewer::addKdRender(String uuid, Node* parent, String palette)
{
  if (!parent)
    parent = getRoot();

  if (uuid.empty())
    uuid = dataflow->guessNodeUIID("kdrender");

  dropSelection();

  KdRenderArrayNode* ret = nullptr;
  beginUpdate(
    StringTree("AddKdRender", "uuid", uuid, "parent", getUUID(parent)),
    StringTree("RemoveNode", "uuid", uuid));
  {
    //render
    auto render_node = new KdRenderArrayNode();
    ret = render_node;
    render_node->setName("KdRender");
    render_node->setUUID(uuid);
    addNode(parent, render_node);
    connectNodes(parent, render_node);

    //palette
    if (!palette.empty())
    {
      auto palette_node = new PaletteNode(palette);
      palette_node->setUUID(uuid, "palette");
      palette_node->setName("Palette");
      addNode(render_node, palette_node);
      connectNodes(palette_node, render_node);
    }

  }
  endUpdate();

  return ret;
}


/////////////////////////////////////////////////////////////
void Viewer::write(Archive& ar) const
{
  ar.write("version", OpenVisus_VERSION);
  ar.write("git_revision", OpenVisus_GIT_REVISION);

  //first dump the nodes without parent... NOTE: the first one is always the getRoot()
  auto root = dataflow->getRoot();
  VisusAssert(root == dataflow->getNodes()[0]);
  for (auto node : dataflow->getNodes())
  {
    if (node->getParent()) continue;
    StringTree encoded(node->getTypeName());
    node->write(encoded);
    ar.addChild(StringTree("AddNode").addChild(encoded));
  }

  //then the nodes in the tree...important the order! parents before childs
  for (auto node : root->breadthFirstSearch())
  {
    if (node == root) continue;

    StringTree encoded(node->getTypeName());
    node->write(encoded);
    VisusAssert(node->getParent());
    ar.addChild(StringTree("AddNode", "parent", getUUID(node->getParent())).addChild(encoded));
  }

  //connectNodes actions
  for (auto node : dataflow->getNodes())
  {
    for (auto OT = node->outputs.begin(); OT != node->outputs.end(); OT++)
    {
      auto oport = OT->second;
      for (auto IT = oport->outputs.begin(); IT != oport->outputs.end(); IT++)
      {
        auto iport = (*IT);
        ar.addChild(StringTree("ConnectNodes", "from", getUUID(oport->getNode()), "oport", oport->getName(), "iport", iport->getName(), "to", getUUID(iport->getNode())));
      }
    }
  }

  //selection
  if (auto selection = getSelection())
    ar.addChild(StringTree("SetSelection", "value", getUUID(selection)));
}

/////////////////////////////////////////////////////////////
void Viewer::read(Archive& ar)
{
  double version;
  ar.read("version", version, 0.0);

  String git_revision;
  ar.read("git_revision", git_revision);

  for (auto child : ar.getChilds())
  {
    if (!child->isHash())
      this->execute(*child);
  }
}



} //namespace Visus


