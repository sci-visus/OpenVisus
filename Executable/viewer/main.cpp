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
#include <Visus/ModVisus.h>
#include <Visus/Rectangle.h>
#include <Visus/TransferFunctionView.h>
#include <Visus/ArrayStatisticsView.h>
#include <Visus/FieldNode.h>
#include <Visus/GLCameraNode.h>

#include <QApplication>
#include <QDesktopWidget>

////////////////////////////////////////////////////////////////////////
namespace Visus  {


//TestMasterAndSlave
static void TestMasterAndSlave(Viewer* master)
{
  int X=50;
  int Y=50;
  int W=600;
  int H=400;
  master->setGeometry(X,Y,W,H);
  Viewer* slave = new Viewer("slave"); 
  slave->addNetRcv(3333);
  double fix_aspect_ratio = (double)(W) / (double)(H);
  master->addNetSnd("http://localhost:3333", Rectangle2d(0.0, 0.0, 0.5, 1.0), Rectangle2d(X+W, Y, W, H), fix_aspect_ratio);
}

//TestMasterAndTwoSlaves
static void TestMasterAndTwoSlaves(Viewer* master)
{
  int W=1024;
  int H=768;
  master->setGeometry(300,300,600,400);
  Viewer* slave1 = new Viewer("slave1"); slave1->addNetRcv(3333);
  Viewer* slave2 = new Viewer("slave2"); slave2->addNetRcv(3334);
  int w = W / 2; int ox = 50;
  int h = H    ; int oy = 50;
  double fix_aspect_ratio = (double)(W) / (double)(H);
  master->addNetSnd("http://localhost:3333", Rectangle2d(0.0, 0.0, 0.5, 1.0), Rectangle2d(ox    , oy, w, h), fix_aspect_ratio);
  master->addNetSnd("http://localhost:3334", Rectangle2d(0.5, 0.0, 0.5, 1.0), Rectangle2d(ox + w, oy, w, h), fix_aspect_ratio);
}

//TestMasterAndFourSlaves
static void TestMasterAndFourSlaves(Viewer* master)
{
  int W = (int)(0.8*QApplication::desktop()->width());
  int H = (int)(0.8*QApplication::desktop()->height());

  master->setGeometry(W/2-300,H/2-200,600,400);
    
  Viewer* slave1 = new Viewer("slave1");  slave1->addNetRcv(3333);
  Viewer* slave2 = new Viewer("slave2");  slave2->addNetRcv(3334);
  Viewer* slave3 = new Viewer("slave3");  slave3->addNetRcv(3335);
  Viewer* slave4 = new Viewer("slave4");  slave4->addNetRcv(3336);

  int w = W / 2; int ox = 50;
  int h = H / 2; int oy = 50;
  double fix_aspect_ratio = (double)(w * 2) / (double)(h * 2);
  master->addNetSnd("http://localhost:3333", Rectangle2d(0.0, 0.0, 0.5, 0.5), Rectangle2d(ox    , oy + h, w, h), fix_aspect_ratio);
  master->addNetSnd("http://localhost:3334", Rectangle2d(0.5, 0.0, 0.5, 0.5), Rectangle2d(ox + w, oy + h, w, h), fix_aspect_ratio);
  master->addNetSnd("http://localhost:3335", Rectangle2d(0.5, 0.5, 0.5, 0.5), Rectangle2d(ox + w, oy    , w, h), fix_aspect_ratio);
  master->addNetSnd("http://localhost:3336", Rectangle2d(0.0, 0.5, 0.5, 0.5), Rectangle2d(ox    , oy    , w, h), fix_aspect_ratio);
}

//TestMasterMiddleAndSlave
static void TestMasterMiddleAndSlave(Viewer* master)
{
  master->setGeometry(650,50,600,400);

  Viewer* middle=new Viewer("middle");
  middle->addNetRcv(3333);
    
  Viewer* slave=new Viewer("slave");
  slave->addNetRcv(3334);

  master->addNetSnd("http://localhost:3333", Rectangle2d(0, 0, 1, 1), Rectangle2d(50, 500, 600, 400), 0.0);
  middle->addNetSnd("http://localhost:3334", Rectangle2d(0, 0, 1, 1), Rectangle2d(650, 500, 600, 400), 0.0);
}

} //namespace Visus


////////////////////////////////////////////////////////////////////////
int main(int argn,const char* argv[])
{
  using namespace Visus;

  SetCommandLine(argn, argv);
  GuiModule::createApplication();
  AppKitModule::attach();

  std::vector<String> args=ApplicationInfo::args;
  for (int I=1;I<(int)args.size();I++)
  {
    if (args[I]=="--help")
    {
      VisusInfo()<<std::endl
        <<"visusviewer help:"<<std::endl
        <<"   --visus-config <path>                                                  - path to visus.config"<<std::endl
        <<"   --open <url>                                                           - opens the specified url or .idx volume"<<std::endl
        <<"   --server [http]                                                        - starts a standalone ViSUS Server on port 10000"<<std::endl
        <<"   --fullscseen                                                           - starts in fullscreen mode"<<std::endl
        <<"   --geometry \"<x> <y> <width> <height>\"                                - specify viewer windows size and location"<<std::endl
        <<"   --zoom-to \"x1 y1 x2 y2\"                                              - set glcamera ortho params" << std::endl
        <<"   --network-rcv <port>                                                   - run powerwall slave"<<std::endl
        <<"   --network-snd <slave_url> <split_ortho> <screen_bounds> <aspect_ratio> - add a slave to a powerwall master"<<std::endl
        <<"   --split-ortho \"x y width height\"                                     - for taking snapshots"<<std::endl
        <<"   --internal-network-test-(11|12|14|111)                                 - internal use only"<<std::endl
        <<std::endl
        <<std::endl;

      AppKitModule::detach();
      return 0;
    }
  }

  UniquePtr<Viewer> viewer(new Viewer());

  typedef Visus::Rectangle2d Rectangle2d;
  String arg_split_ortho;
  String args_zoom_to;

  String open_filename=Dataset::getDefaultDatasetInVisusConfig();
  bool bStartServer=false;
  bool bFullScreen=false;
  Rectangle2i geometry(0, 0, 0, 0);
  String fieldname;
  bool bMinimal = false;

  for (int I=1;I<(int)args.size();I++)
  {
    if (args[I]=="--open") 
    {
      open_filename=args[++I];
    }
    else if (args[I]=="--server") 
    {
      bStartServer = true;
    }
    else if (args[I]=="--fullscreen")
    {
      bFullScreen=true;
    }
    else if (args[I]=="--geometry") 
    {
      geometry=Rectangle2i(args[++I]);
    }
    else if (args[I]=="--fieldname")
    {
      fieldname = args[++I];
    }
    else if (args[I] == "--minimal")
    {
      bMinimal = true;
    }
    else if (args[I]=="--internal-network-test-11")
    {
      TestMasterAndSlave(viewer.get());
    }
    else if (args[I]=="--internal-network-test-12")
    {
      TestMasterAndTwoSlaves(viewer.get());
    }
    else if (args[I]=="--internal-network-test-14")
    {
      TestMasterAndFourSlaves(viewer.get());
    }
    else if (args[I]=="--internal-network-test-111") 
    {
      TestMasterMiddleAndSlave(viewer.get());
    }
    else if (args[I]=="--network-rcv")
    {
      int port=cint(args[++I]);
      viewer->addNetRcv(port);
    }
    else if (args[I]=="--network-snd")
    {
      String url                 = args[++I];
      Rectangle2d split_ortho      = Rectangle2d(args[++I]);
      Rectangle2d screen_bounds    = Rectangle2d(args[++I]);
      double fix_aspect_ratio    = cdouble(args[++I]);
      viewer->addNetSnd(url,split_ortho,screen_bounds,fix_aspect_ratio);
    }
    else if (args[I]=="--split-ortho")
    {
      arg_split_ortho=args[++I];
    }
    else if (args[I] == "--zoom-to")
    {
      args_zoom_to = args[++I];
    }
    //last argment could be a filename. This facilitates OS-initiated launch (e.g. opening a .idx)
    else if (I==(args.size()-1) && !StringUtils::startsWith(args[I], "--"))
    {
      open_filename = args[I];
    }
  }

  if (!open_filename.empty())
  {
    viewer->openFile(open_filename);

    if (!fieldname.empty())
    {
      if (auto fieldnode = viewer->findNodeByType<FieldNode>())
        fieldnode->setFieldName(fieldname);
    }
  }

  if (bMinimal)
  {
    Viewer::Preferences preferences;
    preferences.preferred_panels="";
    preferences.bHideMenus = true;
    viewer->setPreferences(preferences);
  }

  SharedPtr<NetServer> server;
  if (bStartServer)
  {
    //mixing client and server mode for debugging purpouses
    auto modvisus=std::make_shared<ModVisus>();
    modvisus->configureDatasets();
    server=std::make_shared<NetServer>(10000, modvisus);
    server->runInBackground();
  }

  if (bFullScreen)
    viewer->showFullScreen();

  else if (geometry.width>0 && geometry.height>0)
    viewer->setGeometry(geometry.x, geometry.y, geometry.width, geometry.height);

  if (!arg_split_ortho.empty())
  {
    auto split_ortho=Rectangle2d(arg_split_ortho);

    if (auto glcamera = viewer->getGLCamera())
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
    auto world_box = viewer->getWorldBoundingBox();

    if (auto glcamera = viewer->getGLCamera())
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

  GuiModule::execApplication();
  viewer.reset();

  AppKitModule::detach();

  GuiModule::destroyApplication();

  return 0;
}


