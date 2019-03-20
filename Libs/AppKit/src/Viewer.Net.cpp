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

namespace Visus {

////////////////////////////////////////////////////////////
void Viewer::sendNetMessage(SharedPtr<NetConnection> netsnd,void* obj)
{
#if 1
  VisusAssert(false);
#else

  String TypeName=ObjectFactory::getSingleton()->getTypeName(*obj);
  StringTree stree(TypeName);
  ObjectStream ostream(stree, 'w');
  ostream.writeInline("request_id",cstring(++netsnd->request_id));
  obj->writeToObjectStream(ostream);
  ostream.close();
  NetRequest request(netsnd->url);
  request.setTextBody(stree.toString());

  {
    ScopedLock lock(netsnd->requests_lock);
    netsnd->requests.push_back(request);
  }
#endif
}


//////////////////////////////////////////////////////////////////////
bool Viewer::addNetRcv(int port)
{
  String url="http://127.0.0.1:"+cstring(port);
  auto netrcv=std::make_shared<NetConnection>();
  if (!netrcv->socket->bind(url))
  {
    VisusError() << "NetSocket::bind on url("<<url<<") failed";
    VisusAssert(false);
    return false;
  }

  int Id=(int)this->netrcv.size();
  this->netrcv.push_back(netrcv);

  netrcv->url=url;
  netrcv->bSend=false;
  netrcv->log.rdbuf()->pubsetbuf(0,0);
  netrcv->log.rdbuf()->pubsetbuf(0,0);
  netrcv->log.open(KnownPaths::CurrentWorkingDirectory().getChild("netrcv.localhost."+cstring(port)+".txt"));

  netrcv->timer.start(100);

  netrcv->thread=Thread::start("Viewer Net Receiver",[this,Id]()
  {
    auto netrcv=this->netrcv[Id].get();

    netrcv->socket=netrcv->socket->acceptConnection();

    if (!netrcv->socket)
    {
      VisusError() << "cannot accept connection";
      VisusAssert(false);
      return;
    }

    while (!netrcv->bExitThread)
    {
      NetRequest request=netrcv->socket->receiveRequest();
      if (!request.valid())
      {
        VisusError() << "Failed to receive action";
        break;
      }
      else
      {
        ScopedLock lock(netrcv->requests_lock);
        netrcv->requests.push_back(request);
      }
    }

  });
  
  QObject::connect(&netrcv->timer,&QTimer::timeout,[this,Id](){

    auto netrcv=this->netrcv[Id].get();

    std::vector<NetRequest> requests;
    {
      ScopedLock lock(netrcv->requests_lock);
      requests=netrcv->requests;
      netrcv->requests.clear();
    }

    for (auto& request : requests)
    {
      netrcv->log
        << "////////////////////////////////////////"<<std::endl
        << Time::now().getFormattedLocalTime() << " Received request"<< std::endl
        << "////////////////////////////////////////"<<std::endl<<std::endl
        <<request.toString()
        <<std::endl
        <<std::endl;

      StringTree stree;
      if (!stree.fromXmlString(request.getTextBody()))
      {
        VisusAssert(false);
        return;
      }

      ObjectStream istream(stree, 'r');

  #if 1
        VisusAssert(false); //TODO
  #else

      String TypeName = istream.getCurrentContext()->name;
      VisusAssert(!TypeName.empty());
      SharedPtr<Object> obj(ObjectFactory::getSingleton()->createInstance<Object>(TypeName));  VisusAssert(obj);
      obj->readFromObjectStream(istream);
      istream.close();

      if (auto update_glcamera=dynamic_cast<UpdateGLCamera*>(action.get()))
      {
        auto glcamera_node=dynamic_cast<GLCameraNode*>(viewer->findNodeByUUID(update_glcamera->glcamera_node));VisusAssert(glcamera_node);
        update_glcamera->action->redo(viewer->getModel());
        glcamera->setOrthoParams(update_glcamera->ortho_params);
      }
      else
      {
        action->redo(viewer->getModel());
      }
  #endif
    }
  });

  return true;
}

//////////////////////////////////////////////////////////////////////
bool Viewer::addNetSnd(String url,Rectangle2d split_ortho,Rectangle2d screen_bounds,double fix_aspect_ratio)
{
  auto netsnd=std::make_shared<NetConnection>();
  if (!netsnd->socket->connect(url))
  {
    VisusError() << "Failed to connect to " << url << ", closing the connection";
    VisusAssert(false);
    return false;
  }

  int Id=(int)this->netsnd.size();
  this->netsnd.push_back(netsnd);

  netsnd->url =url;
  netsnd->bSend=true;
  netsnd->log.rdbuf()->pubsetbuf(0,0);
  netsnd->log.rdbuf()->pubsetbuf(0,0);
  netsnd->log.open(KnownPaths::CurrentWorkingDirectory().getChild("netsnd."+Url(url).getHostname()+"."+cstring(Url(url).getPort())+".txt").toString().c_str());

  netsnd->split_ortho=split_ortho;
  netsnd->fix_aspect_ratio=fix_aspect_ratio;

  //send the main component preferences
  {
#if 1
    VisusAssert(false); //todo
#else
    UniquePtr<SetMainComponentPreferences> action(new SetMainComponentPreferences("0",Preferences("")));
    action->value.bHideMenus=true;
    action->value.bHideTitleBar=true;
    action->value.screen_bounds=screen_bounds;
    netsnd->sendNetMessage(action.get());
#endif
  }

  //send the current scene
  {
#if 1
    VisusAssert(false); //todo
#else
    UniquePtr<BindModel> action(new BindModel("BindModel",model,false));
    netsnd->sendNetMessage(action.get());
#endif
  }

  netsnd->thread=Thread::start("Viewer Net send",[this,Id](){

    auto netsnd=this->netsnd[Id].get();
    while (!netsnd->bExitThread)
    {
      std::vector<NetRequest> requests;
      {
        ScopedLock lock(netsnd->requests_lock);
        netsnd->requests=netsnd->requests;
        netsnd->requests.clear();
      }

      if (requests.empty())
        Thread::sleep(100);

      for (auto& request : requests)
      {
        //hopefully this is non-blocking see http://stackoverflow.com/questions/3578650/c-socket-does-send-wait-for-recv-to-end
        if (!netsnd->socket->sendRequest(request))
        {
          VisusError() << "Failed to send requests to " << netsnd->url << ". Closing the connection";
          VisusAssert(false);
          netsnd->socket.reset();
          return;
        }

        netsnd->log
          << "////////////////////////////////////////"<<std::endl
          << Time::now().getFormattedLocalTime() << " Sent request"<< std::endl
          << "////////////////////////////////////////"<<std::endl<<std::endl
          <<request.toString()
          <<std::endl
          <<std::endl;
      }
    }

  });

  return true;
}


} //namespace Visus
