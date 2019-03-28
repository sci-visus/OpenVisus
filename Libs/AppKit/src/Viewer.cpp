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

#if 0

class Viewer::UpdateGLCamera : public Action
{
public:

  String               glcamera_node;
  SharedPtr<Action>    action;
  GLOrthoParams        ortho_params;

  //constructor
  UpdateGLCamera(GLCameraNode* glcamera_node=nullptr,SharedPtr<Action> action=SharedPtr<Action>(),GLOrthoParams ortho_params=GLOrthoParams())
    : Action("UpdateGLCamera")
  {
    this->glcamera_node=glcamera_node? glcamera_node->getUUID() : "";
    this->action=action;
    this->ortho_params=ortho_params;
  }

  //destructor
  virtual ~UpdateGLCamera() {
  }

  //writeToObjectStream
  virtual void write(Viewer* viewer,ObjectStream& ostream) override
  {
    Action::writeToObjectStream(viewer,ostream);
    ostream.write("glcamera_node",glcamera_node);
    ostream.writeObject("action",action.get());
    ostream.pushContext("ortho_params");
    ortho_params.writeToObjectStream(ostream);
    ostream.popContext("ortho_params");
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override
  {
    Action::readFromObjectStream(viewer,istream);
    glcamera_node=istream.read("glcamera_node");
    action.reset(istream.readObject<Action>("action"));
    istream.pushContext("ortho_params");
    ortho_params.readFromObjectStream(istream);
    istream.popContext("ortho_params");
  }

};
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<Viewer::Logo> Viewer::OpenScreenLogo(String key, String default_logo)
{
  String filename = VisusConfig::getSingleton()->readString(key + "/filename");
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
  ret->pos.x = StringUtils::contains(key, "Left") ? 0 : 1;
  ret->pos.y = StringUtils::contains(key, "Bottom") ? 0 : 1;
  ret->opacity = cdouble(VisusConfig::getSingleton()->readString(key + "/alpha", "0.5"));
  ret->border = Point2d(10, 10);
  return ret;
};


///////////////////////////////////////////////////////////////////////////////////////////////
Viewer::Viewer(String title) : QMainWindow()
{
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

  this->background_color=Color::parseFromString(VisusConfig::getSingleton()->readString("Configuration/VisusViewer/background_color", Colors::DarkBlue.toString()));

  //logos
  {
    if (auto logo = OpenScreenLogo("Configuration/VisusViewer/Logo/BottomLeft" , ":sci.png"  )) logos.push_back(logo);
    if (auto logo = OpenScreenLogo("Configuration/VisusViewer/Logo/BottomRight", ":visus.png")) logos.push_back(logo);
    if (auto logo = OpenScreenLogo("Configuration/VisusViewer/Logo/TopRight"   , ""          )) logos.push_back(logo);
    if (auto logo = OpenScreenLogo("Configuration/VisusViewer/Logo/TopLeft"    , ""          )) logos.push_back(logo);
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
void Viewer::setMinimal()
{
  Viewer::Preferences preferences;
  preferences.preferred_panels = "";
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

  String open_filename = Dataset::getDefaultDataset();
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
    this->openFile(open_filename);

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

      GLOrthoParams ortho_params = glcamera->getOrthoParams();

      double fix_aspect_ratio = W / H;
      if (fix_aspect_ratio)
        ortho_params.fixAspectRatio(fix_aspect_ratio);

      ortho_params = ortho_params.split(split_ortho);
      glcamera->setOrthoParams(ortho_params);
    }
  }

  if (!args_zoom_to.empty())
  {
    auto world_box = this->getWorldBoundingBox();

    if (auto glcamera = this->getGLCamera())
    {
      GLOrthoParams ortho_params = glcamera->getOrthoParams();

      double x1 = 0, y1 = 0, x2 = 1.0, y2 = 1.0;
      std::istringstream parse(args_zoom_to);
      parse >> x1 >> y1 >> x2 >> y2;

      auto p1 = world_box.getPoint(x1, y1, 0).dropZ();
      auto p2 = world_box.getPoint(x2, y2, 0).dropZ();

      ortho_params = GLOrthoParams(p1.x, p2.x, p1.y, p2.y, ortho_params.zNear, ortho_params.zFar);

      VisusInfo() << std::fixed << "Setting"
        << " ortho_params("
        << ortho_params.left << " " << ortho_params.right << " "
        << ortho_params.bottom << " " << ortho_params.top << " "
        << ortho_params.zNear << " " << ortho_params.zFar << ")"
        << " world_box("
        << world_box.p1.x << " " << world_box.p1.y << " " << world_box.p1.z << " "
        << world_box.p2.x << " " << world_box.p2.y << " " << world_box.p2.z << ")";


      double W = glcamera->getViewport().width;
      double H = glcamera->getViewport().height;
      if (W && H)
      {
        double fix_aspect_ratio = W / H;
        ortho_params.fixAspectRatio(fix_aspect_ratio);
      }

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
  clearHistory();

#ifdef VISUS_DEBUG
  enableLog("visusviewer.log.txt");
#endif

  addNode(new Node("World"));
}

//////////////////////////////////////////////////////////////////////
void Viewer::enableSaveSession()
{
  save_session_timer.reset(new QTimer());
  
  String filename = VisusConfig::getSingleton()->readString("Configuration/VisusViewer/SaveSession/filename", KnownPaths::VisusHome.getChild("viewer_session.xml"));
  
  int every_sec    =cint(VisusConfig::getSingleton()->readString("Configuration/VisusViewer/SaveSession/sec","60")); //1 min!

  //make sure I create a unique filename
  String extension=Path(filename).getExtension();
  if (!extension.empty())
    filename=filename.substr(0,filename.size()-extension.size());  
  filename=filename+"."+Time::now().getFormattedLocalTime()+extension;

  VisusInfo() << "Configuration/VisusViewer/SaveSession/filename " << filename;
  VisusInfo() << "Configuration/VisusViewer/SaveSession/sec " << every_sec;

  connect(save_session_timer.get(),&QTimer::timeout,[this,filename](){
    saveFile(filename,/*bSaveHistory*/false,/*bShowDialogs*/false);
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
      ApplicationStats::io .readValues(true);
      ApplicationStats::net.readValues(true);
      //QApplication::setOverrideCursor(Qt::BusyCursor);
    }
  }

  std::ostringstream out;

  if (running.value)
    out << "Working. "<<"TJOB(" << thread_pool_njobs << ") "<<"NJOB(" << netservice_njobs    << ") ";
  else
    out << "Ready runtime(" <<running.enlapsed<<"sec ";

  out <<"nthreads(" << thread_nrunning   << ") ";

  auto io=ApplicationStats::io.readValues(false);
  out << "IO("
    << StringUtils::getStringFromByteSize(io.nopen ) << "/"
    << StringUtils::getStringFromByteSize(io.rbytes ) << "/"
    << StringUtils::getStringFromByteSize(io.wbytes) << ") ";

  auto net=ApplicationStats::net.readValues(false);
  out << "NET("
    << StringUtils::getStringFromByteSize(net.nopen ) << "/"
    << StringUtils::getStringFromByteSize(net.rbytes ) << "/"
    << StringUtils::getStringFromByteSize(net.wbytes) << ") ";

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
Box3d Viewer::getWorldBoundingBox() const
{
  return getNodeBounds(getRoot()).toAxisAlignedBox();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
Position Viewer::getNodeBounds(Node* node,bool bRecursive) const
{
  VisusAssert(node);

  //special case for QueryNode: its content is really its bounds
  if (auto query=dynamic_cast<QueryNode*>(node))
    return query->getNodeBounds();

  //NOTE: the result in in local geometric coordinate system of node 
  {
    Position ret=node->getNodeBounds();
    if (ret.valid()) 
      return ret;
  }

  Matrix T;

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
    return Position(T,getNodeBounds(childs[0],true));
  }
  else
  {
    Box3d box= Box3d::invalid();
    for (auto child : childs)
    {
      Position child_bounds=getNodeBounds(child,true); 
      if (child_bounds.valid())
        box=box.getUnion(child_bounds.toAxisAlignedBox());
    }
    return Position(T,box);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Position Viewer::getNodeBoundsInAnotherSpace(Node* dst,Node* src) const
{
  VisusAssert(dst && src && dst!=src);
  
  Position bounds=getNodeBounds(src);

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
Frustum Viewer::computeNodeFrustum(Frustum frustum,Node* node) const
{
  for (auto it : node->getPathFromRoot())
  {
    if (auto modelview_node=dynamic_cast<ModelViewNode*>(it))
    {
      Matrix T=modelview_node->getModelview();
      frustum.multModelview(T);
    }
  }

  return frustum;
}

//////////////////////////////////////////////////////////////////////
Position Viewer::getQueryBoundsInDatasetSpace(QueryNode* query_node) const
{
  return getNodeBoundsInAnotherSpace(query_node->getDatasetNode(),query_node);
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
    Frustum  frustum = computeNodeFrustum(getGLCamera()->getFrustum(),node);
    Position bounds  = getNodeBounds(node);

    double query_distance=frustum.computeDistance(bounds,screen_point,/*bUseFarPoint*/false);
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
  Position bounds=query_node->getNodeBounds();
  if (!bounds.valid())
  {
    free_transform.reset();
    widgets.glcanvas->postRedisplay();
    return;
  }

  if (!free_transform)
  {
    free_transform=std::make_shared<FreeTransform>();

    free_transform->objectChanged.connect([this,query_node](Position query_pos)
    {
      auto T  =query_pos.getTransformation();
      auto box=query_pos.getBox();

      TRSMatrixDecomposition trs(T);

      if (trs.rotate.getAngle()==0)
      {
        T=Matrix::identity();
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

      query_pos=Position(T,box);
      query_node->setNodeBounds(query_pos);
      free_transform->setObject(query_pos);
      refreshData(query_node);
    });

  }

  free_transform->setObject(bounds);
  postRedisplay();
}

///////////////////////////////////////////////////////////////////////////////
void Viewer::beginFreeTransform(ModelViewNode* modelview_node)
{
  auto T=modelview_node->getModelview();
  auto bounds=getNodeBounds(modelview_node);

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
      Matrix T=obj.getTransformation() * bounds.getTransformation().invert();
      modelview_node->setModelview(T);
      refreshData(modelview_node);
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
    //overwrite the position, need to actualize it to the dataset since QueryNode works in dataset reference space
    auto position=getQueryBoundsInDatasetSpace(query_node);
    query_node->setQueryPosition(position);

    //overwrite the viewdep frustum, since QueryNode works in dataset reference space
    //NOTE: using the FINAL frusutm
    auto viewdep=computeNodeFrustum(getGLCamera()->getFinalFrustum(),query_node->getDatasetNode());
    query_node->setViewDep(viewdep);
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
    if (preferences.preferred_panels.empty())
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
    String filename=cstring(QFileDialog::getOpenFileName(nullptr,"Choose a file to open...",last_dir.c_str(),"*"));
    if (filename.empty()) return;
    last_dir=Path(filename).getParent();
    VisusConfig::getSingleton()->filename=filename;
  }

  bool bForce = true;
  VisusConfig::getSingleton()->reload(bForce);

  widgets.toolbar->bookmarks_button->setMenu(createBookmarks());
}
 
//////////////////////////////////////////////////////////////////////
bool Viewer::openFile(String url,Node* parent,bool bShowUrlDialogIfNeeded)
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
    StringTree stree;
    if (!stree.fromXmlString(Utils::loadTextDocument(url)))
    {
      VisusAssert(false);
      return false;
    }

    for (int i=0;i<stree.getNumberOfChilds();i++)
      VisusConfig::getSingleton()->addChild(stree.getChild(i));

    //load first bookmark from new config
    auto children=stree.findAllChildsWithName("dataset",true);
    if (!children.empty())
    {
      url=children[0]->readString("name",children[0]->readString("url"));VisusAssert(!url.empty());
      return this->openFile(url,parent,false);
    }

    return true;
  }

  if (StringUtils::endsWith(url,".scn") || StringUtils::contains(url, "scene="))
  {
    return openScene(url);
  }
  
  //xml means a Viewer dataflow
  if (StringUtils::endsWith(url,".xml"))
  {
    StringTree stree;
    if (!stree.fromXmlString(Utils::loadTextDocument(url)))
    {
      VisusAssert(false);
      return false;
    }

    setDataflow(std::make_shared<Dataflow>());
    clearHistory();

    try
    {
      ObjectStream stream(stree, 'r');
      this->readFromObjectStream(stream);
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

    VisusInfo() << "openFile(" << url << ") done";
    widgets.treeview->expandAll();
    refreshActions();
    return true;
  }

  //gidx means group of idx: open them all
  if (StringUtils::endsWith(url,".gidx"))
  {
    StringTree stree;
    if (!stree.fromXmlString(Utils::loadTextDocument(url)))
    {
      VisusAssert(false);
      return false;
    }

    for (int i=0;i<stree.getNumberOfChilds();i++)
    {
      if (stree.getChild(i).name!="dataset")
        continue;

      String dataset(stree.getChild(i).readString("url"));
      bool success=this->openFile(dataset,i==0?parent:getRoot(),false);
      if (!success)
        VisusWarning()<<"Unable to open "<<dataset<<" from "<<url;
    }

    widgets.treeview->expandAll();
    refreshActions();
    VisusInfo() << "openFile(" << url << ") done";
    return true;
  }

  //try to open a dataset
  auto dataset = Dataset::loadDataset(url);
  if (!dataset)
  {
    QMessageBox::information(this, "Error", (StringUtils::format() << "open file(" << url << +") failed.").str().c_str());
    return false;
  }

  if (!parent)
  {
    //try to use the default scene, if any
    String default_scene = dataset->getDefaultScene();
    if (!default_scene.empty() && openScene(default_scene)) 
      return true;
    
    //instead if it fails continue opening the dataset
    New();
    parent = dataflow->getRoot();
  }
  
  beginUpdate();
  {
    //add new GLCameraNode
    auto glcamera=getGLCamera();
    if (!glcamera)
    {
      int world_dim=dataset->getPointDim();
      if (world_dim==3)
        glcamera=std::make_shared<GLLookAtCamera>();
      else
        glcamera=std::make_shared<GLOrthoCamera>();

      addGLCameraNode(glcamera);
    }

    addDatasetNode(dataset,parent);

    glcamera->guessPosition(getWorldBoundingBox());

    if (dataset && StringUtils::contains(dataset->getConfig().readString("mirror"), "x"))
      glcamera->mirror(0);

    if (dataset && StringUtils::contains(dataset->getConfig().readString("mirror"), "y"))
      glcamera->mirror(1);

  }
  endUpdate();

  if (widgets.treeview)
    widgets.treeview->expandAll();
  
  refreshActions();
  VisusInfo()<<"openFile("<<url<<") done";
  return true;
}


//////////////////////////////////////////////////////////////////////
bool Viewer::saveFile(String url,bool bSaveHistory,bool bShowDialogs)
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

  StringTree stree(this->getTypeName());
  ObjectStream ostream(stree,'w');
  ostream.run_time_options.setValue("bSaveHistory",cstring(bSaveHistory));

  try
  {
    this->writeToObjectStream(ostream);
    String xmlcontent=stree.toString();
    if (!Utils::saveTextDocument(url,xmlcontent))
    {
      if (bShowDialogs) 
      {
        String errmsg=StringUtils::format()<<"Failed to save file " << url;
        QMessageBox::information(this,"Error",errmsg.c_str());
      }
      return false;
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
  
//////////////////////////////////////////////////////////////////////
bool Viewer::openScene(String url,Node* parent,bool bShowUrlDialogIfNeeded)
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
      std::vector<String> supported_ext;
      supported_ext.push_back(".scn");
      
      //NOTE: the * is for plugins
      static String last_filename="";
      url=cstring(QFileDialog::getOpenFileName(nullptr,"Choose a scene to open...",last_filename.c_str(),StringUtils::join(supported_ext,";*","*").c_str()));
      if (url.empty()) return false;
      last_filename=url;
      url=StringUtils::replaceAll(url,"\\","/");
      if (!StringUtils::startsWith(url,"/")) url="/"+url;
      url="file://" + url;
    }
  }
  
  VisusAssert(!url.empty());
  
  StringTree stree;
  
  // if local scene file load from disk
  if (StringUtils::endsWith(url,".scn"))
  {
    if (!stree.fromXmlString(Utils::loadTextDocument(url)))
    {
      VisusAssert(false);
      return false;
    }
  }
  // if remote, make request to the server
  else if(StringUtils::contains(url, "scene="))
  {
    auto response=NetService::getNetResponse(url);
    if (!response.isSuccessful())
    {
      VisusWarning() << "Unable to get the scene from server URL: " << url;
      return false;
    }
    
    if (!stree.fromXmlString(response.getTextBody()))
    {
      VisusWarning() << "Unable to load scene from URL: " << url;
      return false;
    }
    
  }
  
  // Simple check if this is a valid scene
  // (TODO make more robust)
  bool validScene=false;
  {
    bool hasCamera=false;
    bool hasDataset=false;
    for(auto child : stree.getChilds())
    {
      if(child->name == "camera")
        hasCamera=true;
      if(child->name == "dataset")
        hasDataset=true;
    }
    
    validScene=(hasCamera && hasDataset);
  }
  
  if(!validScene)
  {
    VisusWarning() << "Invalid scene at URL: " << url;
    return false;
  }

  setDataflow(std::make_shared<Dataflow>());
  clearHistory();
  
  try
  {
    auto glcamera=getGLCamera();
    if (!parent)
    {
      New();
      parent = dataflow->getRoot();
    }
    
    for(auto child : stree.getChilds())
    {
      //VisusInfo() << "child " << child->name;
      
      if(child->name == "camera"){
        // TODO now assuming only one keyframe
        auto cam = child->findAllChildsWithName("keyframe", true);
        if(cam.size()>0)
        {
          ObjectStream stream(*cam[0], 'r');
          stream.setSceneMode(true);
          beginUpdate();
          if (!glcamera)
          { String cam_type = child->readString("type");
            if(cam_type=="lookAt")
            {
              glcamera=std::make_shared<GLLookAtCamera>();
              glcamera->readFromObjectStream(stream);
            }
            else if(cam_type=="ortho")
            {
              glcamera=std::make_shared<GLOrthoCamera>();
              glcamera->readFromObjectStream(stream);
            }
            addGLCameraNode(glcamera);
          }
          endUpdate();
        }
      }
      else if(child->name == "dataset"){
        
        String dataset_url=child->readString("url");
      
        // if scene uses relative paths
        if(StringUtils::startsWith(dataset_url, "."))
        {
          int dir_idx = (int)url.find_last_of("/");
          dataset_url = url.substr(0,dir_idx) + "/" + dataset_url;
        }
        
        auto dataset=Dataset::loadDataset(dataset_url);
        
        if (!dataset)
        {
          QMessageBox::information(this, "Error", (StringUtils::format() << "open file(" << url << +") failed.").str().c_str());
          return false;
        }
        
        beginUpdate();
        
        DatasetNode* dataset_node=addDatasetNode(dataset,parent);
      
        TimeNode* time_node=NULL;
        QueryNode* query_node=NULL;
        FieldNode* field_node=NULL;
        PaletteNode* pal_node=NULL;
        Node* rend_node=NULL;
        
        dataset_node->setDataset(dataset, true);
        if (!dataset_node->getDataset()->openFromUrl(dataset_url))
          ThrowException(StringUtils::format()<<"Cannot open dataset from url "<<dataset_url);
        
        for(auto data_child : dataset_node->breadthFirstSearch())
        {
          if (auto tmp=dynamic_cast<TimeNode*>(data_child))
            time_node = tmp;
          else if (auto tmp = dynamic_cast<QueryNode*>(data_child))
            query_node = tmp;
          else if (auto tmp = dynamic_cast<FieldNode*>(data_child))
            field_node = tmp;
          else if (auto tmp = dynamic_cast<PaletteNode*>(data_child))
            pal_node = tmp;
          else if (auto tmp = dynamic_cast<RenderArrayNode*>(data_child))
            rend_node = tmp;
          else if (auto tmp = dynamic_cast<OSPRayRenderNode*>(data_child))
            rend_node = tmp;
        }

        auto query=child->findChildWithName("query");
        if(query_node != NULL)
        {
          ObjectStream query_stream(*query, 'r');
          query_stream.setSceneMode(true);
          query_node->readFromObjectStream(query_stream);
          query_node->setName(child->readString("name"));
        }
        
        auto field=child->findChildWithName("field");
        if(field_node != NULL)
        {
          ObjectStream field_stream(*field, 'r');
          field_stream.setSceneMode(true);
          field_node->readFromObjectStream(field_stream);
        }
        
        auto pal=child->findChildWithName("palette");
        if(pal_node != NULL)
        {
          ObjectStream pal_stream(*pal, 'r');
          pal_stream.setSceneMode(true);
          pal_node->getPalette()->readFromObjectStream(pal_stream);
        }
      
        auto rend=child->findChildWithName("render");
        if(rend_node != NULL)
        {
          ObjectStream rend_stream(*rend, 'r');
          rend_stream.setSceneMode(true);
          rend_node->readFromObjectStream(rend_stream);
        }
        
        auto time=child->findChildWithName("timestep");
        if(time != NULL)
        { // TODO support multiple keyframes
          auto key0 = time->findAllChildsWithName("time", true)[0];
          double time_value = 0.0;
          StringUtils::tryParse(key0->readString("value"), time_value);
          time_node->setCurrentTime(time_value);
        }
      
        endUpdate();
      }
    }
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
  
  VisusInfo() << "openFile(" << url << ") done";
  widgets.treeview->expandAll();
  refreshActions();
  return true;
  
  
//  widgets.treeview->expandAll();
//  refreshActions();
//  VisusInfo()<<"openFile("<<url<<") done";
//  return true;
}

//////////////////////////////////////////////////////////////////////
bool Viewer::saveScene(String url, bool bShowDialogs)
{
  if (url.empty() && bShowDialogs)
  {
    static String last_dir(KnownPaths::VisusHome.toString());
    url=cstring(QFileDialog::getSaveFileName(nullptr,"Choose a file to save...",last_dir.c_str(),"*.scn"));
    if (url.empty()) return false;
    last_dir=Path(url).getParent();
  }
  
  //add default extension
  if (Path(url).getExtension().empty())
    url=url+".scn";
  
  StringTree stree("scene");
  ObjectStream ostream(stree,'w');
  ostream.setSceneMode(true);
  ostream.writeInline("version", "0");
  
  try
  {
    // Here we will use an AnimationTimeline object which will contain
    // all the keyframes that the user created
    
    auto root = dataflow->getRoot();
    VisusAssert(root == dataflow->getNodes()[0]);
    
    // TODO get animation information from "AnimationTimeline"
    // for now we have only one keyframe
    ostream.pushContext("animation");
    ostream.writeInline("start", "0");
    ostream.writeInline("end", "0");
    ostream.popContext("animation");
    
    //then the root children
    for (auto* node : root->getChilds())
    {
      String TypeName = NodeFactory::getSingleton()->getTypeName(*node);
      
      if (TypeName == "GLCameraNode")
      {
        GLCameraNode* cam_node = (GLCameraNode*)node;
        cam_node->writeToObjectStream(ostream);
      }
      else if(TypeName == "DatasetNode")
      {
        DatasetNode* data_node = (DatasetNode*)node;
      
        ostream.pushContext("dataset");
        
        TimeNode* time_node = NULL;
        QueryNode* query_node = NULL;
        FieldNode* field_node = NULL;
        PaletteNode* pal_node = NULL;
        Node* rend_node = NULL;
        
        for(auto child : node->breadthFirstSearch())
        {
          String child_TypeName = NodeFactory::getSingleton()->getTypeName(*child);

          if (auto tmp = dynamic_cast<TimeNode*>(child))
            time_node = tmp;

          else if (auto tmp = dynamic_cast<QueryNode*>(child))
            query_node = tmp;

          else if (auto tmp = dynamic_cast<FieldNode*>(child))
            field_node = tmp;

          else if (auto tmp = dynamic_cast<PaletteNode*>(child))
            pal_node = tmp;

          else if (auto tmp = dynamic_cast<RenderArrayNode*>(child))
            rend_node = tmp;

          else if (auto tmp = dynamic_cast<OSPRayRenderNode*>(child))
            rend_node = tmp;

          //ostream.write("node", TypeName);
        }
        
        ostream.writeInline("name", query_node->getName());
        ostream.writeInline("url", data_node->getDataset()->getUrl().toString());
        
        if(field_node != NULL)
          field_node->writeToObjectStream(ostream);
        
        if(query_node != NULL)
          query_node->writeToObjectStream(ostream);
        
        if(time_node != NULL)
          time_node->writeToObjectStream(ostream);
        
        if(pal_node != NULL)
          pal_node->getPalette()->writeToObjectStream(ostream);
        
        if(rend_node != NULL)
          rend_node->writeToObjectStream(ostream);
        
        ostream.popContext("dataset");
      }
    }
    
    String xmlcontent=stree.toString();
    if (!Utils::saveTextDocument(url,xmlcontent))
    {
      if (bShowDialogs)
      {
        String errmsg=StringUtils::format()<<"Failed to save file " << url;
        QMessageBox::information(this,"Error",errmsg.c_str());
      }
      return false;
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
void Viewer::setAutoRefresh(AutoRefresh value)  {

  value.timer.reset();

  beginUpdate();
  {
    this->auto_refresh = value;
    widgets.toolbar->auto_refresh.check->setChecked(value.bEnabled);
    widgets.toolbar->auto_refresh.msec->setText(cstring(value.msec).c_str());
  }
  endUpdate();

  if (auto_refresh.bEnabled && auto_refresh.msec)
  {
    this->auto_refresh.timer = std::make_shared<QTimer>();
    QObject::connect(this->auto_refresh.timer.get(), &QTimer::timeout, [this]() {
      refreshData();
    });
    this->auto_refresh.timer->start(this->auto_refresh.msec);
  }

}


////////////////////////////////////////////////////////////////////
class Viewer::DropProcessing : public Action
{
public:

  //constructor
  DropProcessing() : Action("DropProcessing") {
  }

  //destructor
  virtual ~DropProcessing() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    viewer->dropProcessing();
  }

  //undo
  virtual void undo(Viewer* viewer) override{
    viewer->dropProcessing();
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override {
    Action::write(viewer,ostream);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override {
    Action::read(viewer,istream);
  }
};


////////////////////////////////////////////////////////////////////
void Viewer::dropProcessing()
{
  pushAction(std::make_shared<DropProcessing>());
  {
    dataflow->abortProcessing();
    dataflow->joinProcessing();
  }
  popAction();

  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::SetFastRendering : public Action
{
public:

  bool value;

  //constructor
  SetFastRendering(bool value_=false) : Action("SetFastRendering"),value(value_) {
  }
  
  //destructor
  virtual ~SetFastRendering() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    viewer->setFastRendering(value);
  }

  //undo
  virtual void undo(Viewer* viewer) override {
    viewer->setFastRendering(!value);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override {
    Action::write(viewer,ostream);
    ostream.writeInline("value",cstring(value));
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override {
    Action::read(viewer,istream);
    this->value=cbool(istream.readInline("value"));
  }

};

void Viewer::setFastRendering(bool value)
{
  if (bFastRendering==value)  return;
  pushAction(std::make_shared<SetFastRendering>(value));
  bFastRendering=value;
  popAction();
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::SetSelection : public Action
{
public:

  String value;
  String old_value;

  //constructor
  SetSelection(Node* old_value=nullptr,Node* value=nullptr) 
  : Action("SetSelection"){
    this->old_value = old_value? old_value->getUUID() : "";
    this->value     =     value? value    ->getUUID() : "";
  }

  //destructor
  virtual ~SetSelection() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    Node* value    =viewer->findNodeByUUID(this->value);
    viewer->setSelection(value);
  }

  //undo
  virtual void undo(Viewer* viewer) override {
    Node* old_value=viewer->findNodeByUUID(this->old_value);
    viewer->setSelection(old_value);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override 
  {
    Action::write(viewer,ostream);
    ostream.writeInline("value",value);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override 
  {
    Action::read(viewer,istream);
    this->value=istream.readInline("value");
  }

};


void Viewer::setSelection(Node* new_selection)
{
  auto old_selection=getSelection();

  if (old_selection==new_selection)  
    return;

  pushAction(std::make_shared<SetSelection>(old_selection,new_selection));
  dataflow->setSelection(new_selection);
  popAction();

  //in case there is an old free transform going...
  endFreeTransform();

  if (auto query_node=dynamic_cast<QueryNode*>(new_selection))
    beginFreeTransform(query_node);
    
  else if (auto modelview_node=dynamic_cast<ModelViewNode*>(new_selection))
    beginFreeTransform(modelview_node);

  refreshActions();

  postRedisplay();
}

//////////////////////////////////////////////////////////
void Viewer::setName(Node* node,String value)
{
  if (node->getName()==value) 
    return;

  beginUpdate();
  node->setName(value);
  endUpdate();
  postRedisplay();
}

//////////////////////////////////////////////////////////
void Viewer::setHidden(Node* node,bool value)
{
  if (node->isHidden()==value) 
    return;

  beginUpdate();
  dropProcessing();
  for (auto it : node->breadthFirstSearch())
    node->setHidden(value);
  endUpdate();

  refreshActions();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////
class Viewer::ChangeNode : public Action 
{
public:

  String               node;
  std::vector<String>  before;
  std::vector<String>  after;
  Diff                 diff;

  //constructorf
  ChangeNode(String node_="") : Action("ChangeNode"),node(node_){
  }

  //destructor
  virtual ~ChangeNode(){
  }

  //setBefore
  void setBefore(Node* node) 
  {
    StringTree stree(node->getTypeName());
    ObjectStream ostream(stree,'w');
    node->writeToObjectStream(ostream);
    ostream.close();
    this->before=StringUtils::getNonEmptyLines(stree.toXmlString());
  }

  //setAfter
  void setAfter(Node* node) 
  {
    StringTree stree(node->getTypeName());
    ObjectStream ostream(stree, 'w');
    node->writeToObjectStream(ostream);
    ostream.close();
    this->after = StringUtils::getNonEmptyLines(stree.toXmlString());
    this->diff = Diff(this->before, this->after);
    this->before.clear();
    this->after .clear();
  }

  //redo
  virtual void redo(Viewer* viewer) override 
  {
    auto node=viewer->findNodeByUUID(this->node); VisusAssert(node);
    diff.applyToTarget(node,true);
  }

  //undo
  virtual void undo(Viewer* viewer) override 
  {
    auto node=viewer->findNodeByUUID(this->node); VisusAssert(node);
    diff.applyToTarget(node,false);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override
  {
    Action::write(viewer,ostream);
    ostream.writeInline("node",this->node);

    if (!diff.empty())
    {
      ostream.pushContext("diff");
      ostream.writeText(this->diff.toString(),/*bCData*/true);
      ostream.popContext("diff");
    }
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override
  {
    Action::read(viewer,istream);
    this->node=istream.readInline("node");
    if (istream.pushContext("diff"))
    {
      auto lines = StringUtils::getNonEmptyLines(istream.readText());
      this->diff=Diff(lines);
      istream.popContext("diff");
    }
  }

};



////////////////////////////////////////////////////////////////////
class Viewer::AddNode : public Action
{
public:

  String parent;
  String node;
  int    index=-1;
  
  SharedPtr<StringTree> encoded;

  //constructor
  AddNode(Node* parent=nullptr,Node* node=nullptr,int index=-1) 
    : Action("AddNode")
  {
    this->parent=parent? parent->getUUID() : "";
    this->index=index;
    this->node=node? node->getUUID() : "" ;

    if (node) 
    {
      this->encoded = std::make_shared<StringTree>(node->getTypeName());
      ObjectStream ostream(*this->encoded, 'w');
      node->writeToObjectStream(ostream);
      ostream.close();
      VisusAssert(this->encoded->readString("uuid")==this->node);
    }
  }

  //destructor
  virtual ~AddNode() {
  }

  //redo
  virtual void redo(Viewer* viewer) override 
  {
    Node* parent=viewer->findNodeByUUID(this->parent);
    VisusAssert(encoded);

    ObjectStream istream(*encoded, 'r');
    auto TypeName = istream.readInline("TypeName");
    auto node=NodeFactory::getSingleton()->createInstance(TypeName); VisusAssert(node);
    node->readFromObjectStream(istream);
    istream.close();

    VisusAssert(node && node->getUUID()==this->node);
    viewer->addNode(parent,node,index); //this will create a new AddNode with encode_node inside
  }

  //undo
  virtual void undo(Viewer* viewer) override 
  {
    VisusAssert(encoded);
    Node* node=viewer->findNodeByUUID(this->node);VisusAssert(node);
    viewer->removeNode(node);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override 
  {
    Action::write(viewer,ostream);

    VisusAssert(encoded);

    if (!parent.empty())
      ostream.writeInline("parent",parent);
  
    if (index!=-1)
      ostream.writeInline("index",cstring(index));

    ostream.getCurrentContext()->addChild(*encoded);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override 
  {
    Action::read(viewer,istream);
    VisusAssert(!encoded);

    this->parent=istream.readInline("parent");
    this->index=cint(istream.readInline("index","-1"));

    VisusAssert(istream.getCurrentContext()->getNumberOfChilds()==1);
    this->encoded=std::make_shared<StringTree>(istream.getCurrentContext()->getChild(0));
    this->node=encoded->readString("uuid");VisusAssert(!this->node.empty());
  }

};



void Viewer::addNode(Node* parent,Node* node,int index)
{
  if (!node)
    return;

  node->begin_update.connect([this,node](){
    auto change_node=std::make_shared<ChangeNode>(node->getUUID());
    change_node->setBefore(node);
    pushAction(change_node);
  });

  node->changed.connect([this,node](){
    auto change_node=std::dynamic_pointer_cast<ChangeNode>(getTopAction()); VisusAssert(change_node);
    change_node->setAfter(node);
    popAction();
  });

  beginUpdate();
  {
    dropSelection();
    pushAction(std::make_shared<AddNode>(parent,node,index));
    dataflow->addNode(parent,node,index);
    popAction();
  }
  endUpdate();
  
  if (auto glcamera_node=dynamic_cast<GLCameraNode*>(node))
    attachGLCamera(glcamera_node->getGLCamera());
  
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::RemoveNode : public Action
{
public:

  String                parent;
  int                   index;
  String                node;

  SharedPtr<StringTree> encoded;

  //destructor
  RemoveNode(Node* node=nullptr) 
    : Action("RemoveNode")
  {
    this->node  =node? node->getUUID() : "";
    this->parent=node? node->getParent()->getUUID() : "";
    this->index =node? node->getIndexInParent() : -1;

    if (node) 
    {
      this->encoded = std::make_shared<StringTree>(node->getTypeName());
      ObjectStream ostream(*this->encoded, 'w');
      node->writeToObjectStream(ostream);
      ostream.close();
      VisusAssert(encoded);
    }
  }

  //destructor
  virtual ~RemoveNode() {
  }

  //redo
  virtual void redo(Viewer* viewer) override 
  {
    Node* node=viewer->findNodeByUUID(this->node);
    VisusAssert(node);
    viewer->removeNode(node); //this will create a new RemoveNode with encoded inside
  }

  //undo
  virtual void undo(Viewer* viewer) override 
  {
    VisusAssert(encoded);

    ObjectStream istream(*encoded, 'r');
    auto TypeName = istream.readInline("TypeName");
    auto node = NodeFactory::getSingleton()->createInstance(TypeName); VisusAssert(node);
    node->readFromObjectStream(istream);
    istream.close();

    Node* parent=viewer->findNodeByUUID(this->parent);
    VisusAssert((this->parent.empty() && !parent) || (!this->parent.empty() && parent));
    viewer->addNode(parent,node,this->index);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override 
  {
    Action::write(viewer,ostream);
    ostream.writeInline("node",node);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override 
  {
    Action::read(viewer,istream);
    node=istream.readInline("node");
  }

};

void Viewer::removeNode(Node* NODE)
{
  if (!NODE)
    return;

  beginUpdate();

  dropProcessing();

  dropSelection();

  for (auto node : NODE->reversedBreadthFirstSearch())
  {
    VisusAssert(node->getChilds().empty());

    if (auto glcamera_node=dynamic_cast<GLCameraNode*>(node))
      detachGLCamera();

    for (auto input : node->inputs) 
    {
      DataflowPort* iport=input.second; VisusAssert(iport->outputs.empty());//todo multi dataflow
      while (!iport->inputs.empty())
      {
        auto oport=*iport->inputs.begin();
        disconnectPorts(oport->getNode(),oport->getName(),iport->getName(),iport->getNode());
      }
    }
      
    //disconnect outputs
    for (auto output : node->outputs) 
    {
      auto oport=output.second; VisusAssert(oport->inputs.empty());//todo multi dataflow
      while (!oport->outputs.empty())
      {
        auto iport=*oport->outputs.begin();
        disconnectPorts(oport->getNode(),oport->getName(),iport->getName(),iport->getNode());
      }
    }

    VisusAssert(node->isOrphan());

    pushAction(std::make_shared<RemoveNode>(node));
    {
      //don't care about disconnecting slots, the node is going to be deallocated
      dataflow->removeNode(node);
    }
    popAction();
  }

  autoConnectPorts();

  endUpdate();
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::MoveNode : public Action
{
public:

  String dst;
  String src;
  int    index=-1;

  String old_dst;
  int    old_index=-1;

  //constructor
  MoveNode(Node* dst=nullptr,Node* src=nullptr,int index=-1) 
    : Action("MoveNode")
  {
    this->dst=dst? dst->getUUID() : "";
    this->src=src? src->getUUID() : "";
    this->index=index;

    if (src)
    {
      this->old_dst=src->getParent()->getUUID();
      this->old_index=src->getIndexInParent();
    }
  }

  //destructor
  virtual ~MoveNode() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    Node* src=viewer->findNodeByUUID(this->src);
    Node* dst=viewer->findNodeByUUID(this->dst);
    viewer->moveNode(dst,src,index);
  }

  //undo
  virtual void undo(Viewer* viewer) override {
    Node* src=viewer->findNodeByUUID(this->src);
    Node* old_dst=viewer->findNodeByUUID(this->old_dst);
    viewer->moveNode(old_dst,src,old_index);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override 
  {
    Action::write(viewer,ostream);
    ostream.writeInline("dst",dst);
    ostream.writeInline("src",src);
    if (index!=-1)
      ostream.writeInline("index",cstring(index));
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override 
  {
    Action::read(viewer,istream);
    this->dst=istream.readInline("dst");
    this->src=istream.readInline("src");
    this->index=cint(istream.readInline("index","-1"));
  }
};

void Viewer::moveNode(Node* dst,Node* src,int index)
{
  if (!dataflow->canMoveNode(dst,src))
    return;

  pushAction(std::make_shared<MoveNode>(dst,src,index));
  dataflow->moveNode(dst,src,index);
  popAction();
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::ConnectPorts : public Action
{
public:

  String  from;
  String  oport;
  String  iport;
  String  to;

  //constructor
  ConnectPorts(Node* from=nullptr,String oport="",String iport="",Node* to=nullptr) 
    : Action("ConnectPorts")
  {
    this->from=from? from->getUUID() : "";
    this->oport=oport;
    this->iport=iport;
    this->to=to? to->getUUID() : "";
  }

  //destructor
  virtual ~ConnectPorts() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    Node* from=viewer->findNodeByUUID(this->from);
    Node* to  =viewer->findNodeByUUID(this->to);
    viewer->connectPorts(from,oport,iport,to);
  }

  //undo
  virtual void undo(Viewer* viewer) override {
    Node* from=viewer->findNodeByUUID(this->from);
    Node* to  =viewer->findNodeByUUID(this->to);
    viewer->disconnectPorts(from,oport,iport,to);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override 
  {
    Action::write(viewer,ostream);
    ostream.writeInline("from" ,this->from);
    ostream.writeInline("oport",this->oport);
    ostream.writeInline("iport",this->iport);
    ostream.writeInline("to"   ,this->to);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override
  {
    Action::read(viewer,istream);
    this->from =istream.readInline("from") ; VisusAssert(!from.empty());
    this->oport=istream.readInline("oport"); VisusAssert(!oport.empty());
    this->iport=istream.readInline("iport"); VisusAssert(!iport.empty());
    this->to   =istream.readInline("to")   ; VisusAssert(!to.empty());
  }

};

void Viewer::connectPorts(Node* from,String oport_name,String iport_name,Node* to)
{
  pushAction(std::make_shared<ConnectPorts>(from,oport_name,iport_name,to));
  dataflow->connectPorts(from,oport_name,iport_name,to);
  popAction();
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::DisconnectPorts : public Action
{
public:

  String from;
  String oport;
  String iport;
  String to;

  //constructor
  DisconnectPorts(Node* from=nullptr,String oport="",String iport="",Node* to=nullptr) 
    : Action("DisconnectPorts")
  {
    this->from=from? from->getUUID() : "";
    this->oport=oport;
    this->iport=iport;
    this->to=to? to->getUUID() : "";
  }

  //destructor
  virtual ~DisconnectPorts() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    Node* from=viewer->findNodeByUUID(this->from);
    Node* to  =viewer->findNodeByUUID(this->to);
    viewer->disconnectPorts(from,oport,iport,to);
  }

  //undo
  virtual void undo(Viewer* viewer) override {
    Node* from=viewer->findNodeByUUID(this->from);
    Node* to  =viewer->findNodeByUUID(this->to);
    viewer->connectPorts(from,oport,iport,to);
  }

  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override
  {
    Action::write(viewer,ostream);
    ostream.writeInline("from" ,this->from);
    ostream.writeInline("oport",this->oport);
    ostream.writeInline("iport",this->iport);
    ostream.writeInline("to"  ,this->to);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override
  {
    Action::read(viewer,istream);
    this->from =istream.readInline("from" ); VisusAssert(!from.empty());
    this->oport=istream.readInline("oport"); VisusAssert(!oport.empty());
    this->iport=istream.readInline("iport"); VisusAssert(!iport.empty());
    this->to   =istream.readInline("to"   ); VisusAssert(!to.empty());
  }

};

void Viewer::disconnectPorts(Node* from,String oport_name,String iport_name,Node* to)
{
  pushAction(std::make_shared<DisconnectPorts>(from,oport_name,iport_name,to));
  dataflow->disconnectPorts(from,oport_name,iport_name,to);
  popAction();
  postRedisplay();
}

////////////////////////////////////////////////////////////////////////
void Viewer::autoConnectPorts()
{
  beginUpdate();

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

  endUpdate();
  postRedisplay();
}


////////////////////////////////////////////////////////////////////
class Viewer::RefreshData : public Action
{
public:

  String node;

  //constructor
  RefreshData(Node* node=nullptr) : Action("RefreshData") {
    this->node=node? node->getUUID() : "";
  }

  //destructor
  virtual ~RefreshData() {
  }

  //redo
  virtual void redo(Viewer* viewer) override {
    auto node=viewer->findNodeByUUID(this->node);
    viewer->refreshData(node);
  }

  //undo
  virtual void undo(Viewer* viewer) override{
    auto node=viewer->findNodeByUUID(this->node);
    viewer->refreshData(node);
  }


  //write
  virtual void write(Viewer* viewer,ObjectStream& ostream) override
  {
    Action::write(viewer,ostream);
    ostream.writeInline("node" ,this->node);
  }

  //read
  virtual void read(Viewer* viewer,ObjectStream& istream) override
  {
    Action::read(viewer,istream);
    this->node =istream.readInline("node" ); 
  }
};


/////////////////////////////////////////////////////////////////
void Viewer::refreshData(Node* node)
{
  pushAction(std::make_shared<RefreshData>(node));

  if (node)
  {
    if (auto query_node=dynamic_cast<QueryNode*>(node))
    {
      query_node->setNodeBounds(query_node->getNodeBounds(),/*bForce*/true);
    }
    else if (auto modelview_node=dynamic_cast<ModelViewNode*>(node))
    {
      //find all queries, they could have been their relative position in the dataset changed
      for (auto it : modelview_node->breadthFirstSearch())
      {
        if (auto query_node=dynamic_cast<QueryNode*>(it))
        {
          Position position=getQueryBoundsInDatasetSpace(query_node);
          if (position!=query_node->getQueryPosition())
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

  popAction();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addGroupNode(Node* parent,String name)  
{
  if (!parent)
    parent=getRoot();

  if (name.empty())
  {
    name=cstring(QInputDialog::getText(this,"Insert the group name:","",QLineEdit::Normal,""));
    if (name.empty()) 
      return nullptr;
  }

  auto group_node=new Node(name);
  beginUpdate();
  addNode(parent,group_node,0);
  endUpdate();

  return group_node;
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
GLCameraNode* Viewer::addGLCameraNode(SharedPtr<GLCamera> glcamera,Node* parent) 
{
  if (!parent)
    parent=getRoot();

  auto glcamera_node=new GLCameraNode(glcamera);

  beginUpdate();
  addNode(parent,glcamera_node,/*index*/0);
  endUpdate();

  return glcamera_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IsoContourNode* Viewer::addIsoContourNode(Node* parent,Node* data_provider,double isovalue) 
{
  if (!data_provider)
    data_provider=parent;

  VisusAssert(parent && data_provider && data_provider->hasOutputPort("data"));

  //IsoContourNode
  auto build_isocontour=new IsoContourNode("Marching cube");
  {
    build_isocontour->setIsoValue(isovalue);
  }

  //RenderIsoCountorNode
  auto render_node=new IsoContourRenderNode("Mesh Render");
  {
    GLMaterial material=render_node->getMaterial();
    material.front.diffuse  =Color::createFromUint32(0x3c6d3eff);
    material.front.specular =Color::createFromUint32(0xffffffff);
    material.back.diffuse   =Color::createFromUint32(0x2a4b70ff);
    material.back.specular  =Color::createFromUint32(0xffffffff);
    render_node->setMaterial(material);
  }

  //PaletteNode
  //this is useful if the data coming from the data provider has 2 components, first is used to compute the 
  //marching cube, the second as a second field to color the vertices of the marching cube
  auto palette_node=new PaletteNode("Palette","GrayOpaque");

  beginUpdate();
  {
    dropSelection();

    addNode(parent,build_isocontour);
    addNode(parent,palette_node);
    addNode(parent,render_node);

    connectPorts(data_provider,"data",build_isocontour);
    connectPorts(build_isocontour,"data",render_node);
    connectPorts(build_isocontour,"data",palette_node); //enable statistics
    connectPorts(palette_node,"palette",render_node);
  }
  endUpdate();

  return build_isocontour;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Viewer::addRenderArrayNode(Node* parent,Node* data_provider,String default_palette,String render_type)  
{
  if (!data_provider)
    data_provider=parent;

  VisusAssert(parent && data_provider && data_provider->hasOutputPort("data"));

  Node* render_node;
  
  if (render_type == "ospray" || VisusConfig::getSingleton()->readString("Configuration/VisusViewer/DefaultRenderNode/value")=="ospray")
    render_node = new OSPRayRenderNode("OSPray Render Node");
  else
    render_node = new RenderArrayNode("Render Node");

  auto palette_node=default_palette.empty()? nullptr : new PaletteNode("Palette",default_palette);

  beginUpdate();
  {
    dropSelection();

    if (palette_node)
      addNode(parent,palette_node);

    addNode(parent,render_node);
    connectPorts(data_provider,"data",render_node);

    if (palette_node)
    {
      connectPorts(data_provider,"data",palette_node); //enable statistics
      connectPorts(palette_node,"palette",render_node);
    }
  }
  endUpdate();

  return render_node;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
KdRenderArrayNode* Viewer::addKdRenderArrayNode(Node* parent,Node* data_provider) 
{
  if (!data_provider)
    data_provider=parent;

  VisusAssert(parent && data_provider && data_provider->hasOutputPort("data"));

  auto render_node =new KdRenderArrayNode("KdRender");
  auto palette_node=new PaletteNode("Palette","GrayOpaque");

  beginUpdate();
  {
    dropSelection();
    addNode(parent,palette_node);
    addNode(parent,render_node);
    connectPorts(data_provider,"data",render_node);
    //connectPorts(data_provider,"data",palette_node); enable statistics
    connectPorts(palette_node,render_node);
  }
  endUpdate();

  return render_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
QueryNode* Viewer::addQueryNode(Node* parent,DatasetNode* dataset_node,String name,int dim,String fieldname,int access_id,String rendertype) 
{
  if (!parent)
  {
    parent=findNodeByType<DatasetNode>();
    if (!parent)
      parent=getRoot();
  }

  if (!dataset_node)
  {
    dataset_node=dynamic_cast<DatasetNode*>(parent);
    if (!dataset_node)
      dataset_node= findNodeByType<DatasetNode>();
  }

  VisusAssert(parent && dataset_node);

  auto dataset=dataset_node->getDataset();
  if (!dataset) 
  {
    VisusInfo()<<"Cannot find dataset";
    return nullptr;
  }

  rendertype=StringUtils::toLower(rendertype);

  //name
  if (name.empty())
  {
    String basename;
    if (rendertype=="isocontour") 
      basename="IsoContour";
    else if (dim==3)                              
      basename="Volume";
    else
      basename="Slice"; 

    name=parent->guessUniqueChildName(basename);
  }

  if (fieldname.empty())
    fieldname=dataset->getDefaultField().name;


  int dataset_dim=dataset->getPointDim();

  //QueryNode
  auto query_node=new QueryNode(name);
  query_node->setAccessIndex(access_id);
  query_node->setViewDependentEnabled(true);
  query_node->setProgression(Query::GuessProgression);
  query_node->setQuality(Query::DefaultQuality);

  {
    Box3d box=dataset_node->getNodeBounds().toAxisAlignedBox();
    if (dim==3)
    {
      const double Scale=1.0;
      if (Scale!=1)
        box=box.scaleAroundCenter(Scale);
    }
    else
    {
      VisusAssert(dim==2);
      const int ref=2;
      double Z=box.center()[ref];
      box.p1[ref]=Z;
      box.p2[ref]=Z;
    }
    query_node->setNodeBounds(Position(box));
  }

  //FieldNode
  auto field_node=new FieldNode("fieldnode");
  field_node->setFieldName(fieldname);

  //TimeNode
  auto time_node=dataset_node->findChild<TimeNode*>();
  if (!time_node)
    time_node=new TimeNode("time",dataset->getDefaultTime(),dataset->getTimesteps());

  beginUpdate();
  {
    addNode(parent,query_node);
    addNode(query_node,field_node);

    if (!dataflow->containsNode(time_node))
      addNode(query_node,time_node);

    connectPorts(dataset_node,query_node);
    connectPorts(time_node,query_node);
    connectPorts(field_node,query_node);

    auto scripting_node = new ScriptingNode("Scripting");
    {
      addNode(query_node, scripting_node);
      connectPorts(query_node, "data", scripting_node);
    }

    if (rendertype=="isocontour")
    {
      double isovalue=0.0;
      Field field=dataset->getFieldByName(fieldname);
      if (field.dtype.ncomponents()==1)
      {
        Range range=field.dtype.getDTypeRange();
        isovalue=0.5*(range.from+range.to);
      }
      addIsoContourNode(query_node,scripting_node,isovalue);
    }
    else
    {
      String default_palette_2d=VisusConfig::getSingleton()->readString("Configuration/VisusViewer/default_palette_2d","GrayOpaque");
      String default_palette_3d=VisusConfig::getSingleton()->readString("Configuration/VisusViewer/default_palette_3d","GrayTransparent");
      String default_render_type = VisusConfig::getSingleton()->readString("Configuration/VisusViewer/DefaultRenderNode/value", "");
      addRenderArrayNode(query_node,scripting_node,dim==3?default_palette_3d:default_palette_2d, default_render_type);
    }
  }
  endUpdate();

  return query_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
KdQueryNode* Viewer::addKdQueryNode(Node* parent,DatasetNode* dataset_node,String name,String fieldname,int access_id) 
{
  if (!parent)
  {
    parent= findNodeByType<DatasetNode>();
    if (!parent)
      parent=getRoot();
  }

  if (!dataset_node)
  {
    dataset_node=dynamic_cast<DatasetNode*>(parent);
    if (!dataset_node)
      dataset_node= findNodeByType<DatasetNode>();
  }

  VisusAssert(parent && dataset_node);

  auto dataset=dataset_node->getDataset();
  if (!dataset)
  {
    VisusInfo()<<"Cannot find dataset";
    return nullptr;
  }

  if (name.empty())
    name=parent->guessUniqueChildName("KdQuery");

  int dataset_dim=dataset->getPointDim();

  if (fieldname.empty())
    fieldname=dataset->getDefaultField().name;

  //KdQueryNode
  auto query_node=new KdQueryNode(name);
  query_node->setAccessIndex(access_id);
  query_node->setViewDependentEnabled(true);
  query_node->setQuality(Query::DefaultQuality);

  {
    Box3d box=dataset_node->getNodeBounds().toAxisAlignedBox();
    const double Scale=1.0;
    if (dataset_dim==3 && Scale!=1)  
      box=box.scaleAroundCenter(Scale);
    query_node->setNodeBounds(Position(box));
  }

  //TimeNode
  auto time_node=dataset_node->findChild<TimeNode*>();
  if (!time_node)
    time_node=new TimeNode("time",dataset->getDefaultTime(),dataset->getTimesteps());

  //FieldNode
  auto field_node=new FieldNode("fieldnode");
  field_node->setFieldName(fieldname);

  //render_node
  auto render_node =new KdRenderArrayNode("KdRender");
  auto palette_node=new PaletteNode("Palette","GrayOpaque");

  beginUpdate();
  { 
    addNode(parent,query_node);
    addNode(query_node,field_node);
    addNode(query_node,palette_node);
    addNode(query_node,render_node);

    if (!dataflow->containsNode(time_node))
      addNode(query_node,time_node);

    connectPorts(dataset_node,query_node);
    connectPorts(time_node,query_node);
    connectPorts(field_node,query_node);
    //connectPorts(query_node,"data",palette_node); enable statistics 
    connectPorts(palette_node,"palette",render_node);
    connectPorts(query_node,"data",render_node);
  }
  endUpdate();

  return query_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DatasetNode* Viewer::addDatasetNode(SharedPtr<Dataset> dataset,Node* parent) 
{
  if (!parent)
    parent=getRoot();

  VisusAssert(dataset);

  //add default render node
  int dataset_dim = dataset->getPointDim();

  auto dataset_node= new DatasetNode(dataset->getUrl().toString());
  dataset_node->setDataset(dataset);
  dataset_node->setShowBounds(true);

  //time (this is for queries...)
  auto time_node=new TimeNode("time",dataset->getDefaultTime(),dataset->getTimesteps());

  //rendertype
  String default_rendertype=VisusConfig::getSingleton()->readString("Configuration/VisusViewer/render","");
  String rendertype=StringUtils::toLower(dataset->getConfig().readString("rendertype",default_rendertype));

  beginUpdate();
  {
    dropSelection();

    addNode(parent,dataset_node);
    addNode(dataset_node,time_node);

    if ((dataset->getKdQueryMode() != KdQueryMode::NotSpecified) || rendertype=="kdrender")
    {
      addKdQueryNode(parent,dataset_node);
    }
    else
    {
      addQueryNode(dataset_node, dataset_node, "",/*2*/dataset->getPointDim(), "", 0, rendertype);
    }

    this->refreshData();
  }
  endUpdate();

  return dataset_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ModelViewNode* Viewer::addModelViewNode(Node* parent,bool bInsert)
{
  if (!parent)
    parent=getRoot();

  auto modelview_node=new ModelViewNode("Transform");

  beginUpdate();
  {
    if (bInsert)
    {
      Node* A=parent->getParent();VisusAssert(A);
      Node* B=modelview_node;
      Node* C=parent;
      int index=C->getIndexInParent();
      addNode(A,B,index);
      moveNode(/*dst*/B,/*src*/C);
    }
    else
    {
      addNode(parent,modelview_node);
    }
  }
  endUpdate();
  return modelview_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ScriptingNode* Viewer::addScriptingNode(Node* parent,Node* data_provider) 
{
  if (!parent)
    parent=getRoot();

  if (!data_provider)
    data_provider=parent;

  VisusAssert(data_provider->hasOutputPort("data"));

  auto scripting_node=new ScriptingNode("Scripting");

  beginUpdate();
  {
    dropSelection();
    addNode(parent,scripting_node);
    addRenderArrayNode(parent,scripting_node,"GrayOpaque");

    connectPorts(data_provider,"data",scripting_node);
  }
  endUpdate();

  return scripting_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CpuPaletteNode* Viewer::addCpuTransferFunctionNode(Node* parent,Node* data_provider) 
{
  if (!parent)
    parent=getRoot();

  if (!data_provider)
    data_provider=parent;

  VisusAssert(data_provider->hasOutputPort("data"));

  //CpuPaletteNode
  auto transfer_node=new CpuPaletteNode("CPU Transfer Function");
  {
    //guess number of functions
    int num_functions=1;
    if (DataflowPortValue* last_published=dataflow->guessLastPublished(data_provider->getOutputPort("data")))
    {
      if (auto last_data=std::dynamic_pointer_cast<Array>(last_published->value))
        num_functions=last_data->dtype.ncomponents();
    }

    transfer_node->getTransferFunction()->setNumberOfFunctions(num_functions);
  }

  beginUpdate();
  {
    dropSelection();
    addNode(parent,transfer_node);
    addRenderArrayNode(parent,transfer_node,/*no need for a palette*/"");
    connectPorts(data_provider,"data",transfer_node);
  }
  endUpdate();

  return transfer_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
PaletteNode* Viewer::addPaletteNode(Node* parent,String palette) 
{
  if (!parent)
    parent=getRoot();

  auto palette_node=new PaletteNode("Palette",palette);
  beginUpdate();
  {
    dropSelection();
    addNode(parent,palette_node,1);

    //enable statistics
    if (parent->hasOutputPort("data"))
      connectPorts(parent,"data",palette_node);
  }
  endUpdate();
  return palette_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
StatisticsNode* Viewer::addStatisticsNode(Node* parent,Node* data_provider)  
{
  if (!parent)
    parent = getRoot();

  if (!data_provider)
    data_provider=parent;

  VisusAssert(data_provider->hasOutputPort("data"));
  auto statistics_node=new StatisticsNode("Statistics");
  beginUpdate();
  {
    dropSelection();
    addNode(parent,statistics_node);
    connectPorts(data_provider,"data",statistics_node);
  }
  endUpdate();
  return statistics_node;
}

/////////////////////////////////////////////////////////////
SharedPtr<Viewer::Action> Viewer::createAction(String TypeName)
{
  static std::map< String ,std::function< SharedPtr<Action>() > > creator;
  
  if (creator.empty())
  {
    creator["Transaction"     ]=[](){return std::make_shared<Transaction>();};
    creator["DropProcessing"  ]=[](){return std::make_shared<DropProcessing>();};
    creator["SetFastRendering"]=[](){return std::make_shared<SetFastRendering>();};
    creator["AddNode"         ]=[](){return std::make_shared<AddNode>();};
    creator["RemoveNode"      ]=[](){return std::make_shared<RemoveNode>();};
    creator["MoveNode"        ]=[](){return std::make_shared<MoveNode>();};
    creator["SetSelection"    ]=[](){return std::make_shared<SetSelection>();};
    creator["ConnectPorts"    ]=[](){return std::make_shared<ConnectPorts>();};
    creator["DisconnectPorts" ]=[](){return std::make_shared<DisconnectPorts>();};
    creator["ChangeNode"      ]=[](){return std::make_shared<ChangeNode>();};
    creator["RefreshData"     ]=[](){return std::make_shared<RefreshData>();};
  }

  auto it=creator.find(TypeName); 
  VisusAssert(it!=creator.end());
  return it->second();
}


/////////////////////////////////////////////////////////////
void Viewer::writeToObjectStream(ObjectStream& ostream)
{
  ostream.writeInline("version", cstring(ApplicationInfo::version));
  ostream.writeInline("git_revision", ApplicationInfo::git_revision);

  if (bool bSaveHistory = cbool(ostream.run_time_options.getValue("bSaveHistory")))
  {
    for (auto action : getHistory())
    {
      ostream.pushContext(action->TypeName);
      action->write(target(), ostream);
      ostream.popContext(action->TypeName);
    }
  }
  else
  {
    //first dump the nodes without parent... NOTE: the first one is always the getRoot()
    auto root = dataflow->getRoot();
    VisusAssert(root == dataflow->getNodes()[0]);
    for (auto node : dataflow->getNodes())
    {
      if (node->getParent()) continue;
      ostream.pushContext("AddNode");
      AddNode(nullptr, node, -1).write(this,ostream);
      ostream.popContext("AddNode");
    }

    //then the nodes in the tree...important the order! parents before childs
    for (auto node : root->breadthFirstSearch())
    {
      if (node == root) continue;
      VisusAssert(node->getParent());
      ostream.pushContext("AddNode");
      AddNode(node->getParent(), node, -1).write(this,ostream);
      ostream.popContext("AddNode");
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

          ostream.pushContext("ConnectPorts");
          ConnectPorts(oport->getNode(), oport->getName(), iport->getName(), iport->getNode()).write(this,ostream);
          ostream.popContext("ConnectPorts");
        }
      }
    }

    //selection
    if (auto selection=getSelection())
    {
      ostream.pushContext("SetSelection");
      SetSelection(selection).write(this, ostream);
      ostream.popContext("SetSelection");
    }
  }
}

/////////////////////////////////////////////////////////////
void Viewer::readFromObjectStream(ObjectStream& istream)
{
  double version = cdouble(istream.readInline("version"));
  String git_revision = istream.readInline("git_revision");

  //read all actions
  std::vector< SharedPtr<Action> > actions;
  for (int I = 0; I<istream.getCurrentContext()->getNumberOfChilds(); I++)
  {
    if (istream.getCurrentContext()->getChild(I).isHashNode())
      continue;

    String TypeName = istream.getCurrentContext()->getChild(I).name;
    auto action = this->createAction(TypeName);
    if (!action) 
      continue;
      
    actions.push_back(action);

    istream.pushContext(TypeName);
    action->read(target(), istream);
    istream.popContext(TypeName);
  }

  for (auto action : actions)
    action->redo(target());
}


} //namespace Visus


