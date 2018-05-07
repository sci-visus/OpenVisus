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

#include <Visus/NetService.h>
#include <Visus/NetSocket.h>
#include <Visus/Path.h>
#include <Visus/File.h>
#include <Visus/VisusConfig.h>
#include <Visus/Thread.h>

#include <list>
#include <set>

#if !WIN32
#include <sys/socket.h>
#endif

#include <curl/curl.h>

namespace Visus {

typedef NetService::Connection Connection;


/////////////////////////////////////////////////////////////////////////////
CaCertFile::CaCertFile()
{
  //Downloading up-to-date cacert.pem file from cURL website

  String local_filename = KnownPaths::VisusHome.getChild("cacert.pem");
  String remote_filename = "https://curl.haxx.se/ca/cacert.pem";

#if WIN32
  local_filename = StringUtils::replaceAll(local_filename, "/", "\\");
#endif

  if (!FileUtils::existsFile(local_filename))
  {
    CURL* handle = curl_easy_init();

#if WIN32
    FILE* fp = nullptr;
    fopen_s(&fp, local_filename.c_str(), "wb");
#else
    FILE* fp = fopen(local_filename.c_str(), "wb");
#endif

    if (!fp)
    {
      VisusInfo() << "Could not create " << local_filename << " to store certificate authority.";
      return;
    }

    curl_easy_setopt(handle, CURLOPT_URL, remote_filename.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L); //just for now
    CURLcode res = curl_easy_perform(handle);

    if (res != CURLE_OK) {
      VisusInfo() << "Error getting " + remote_filename;
      return;
    }
    curl_easy_cleanup(handle);
    fclose(fp);

    VisusInfo() << "Saved " << remote_filename << " to " << local_filename;
  }

  VisusInfo() << "Using CaCertFile " << local_filename;
  this->local_filename = local_filename;
}


/////////////////////////////////////////////////////////////////////////////
class CurlNetService : public NetService::Pimpl
{
public:

  
  //_________________________________________________________
  class CurlConnection : public NetService::Connection
  {
  public:

    CurlNetService*    owner;
    CURL*              handle;
    struct curl_slist* slist;
    char               errbuf[CURL_ERROR_SIZE];
    Int64              last_size_download;
    Int64              last_size_upload;
    size_t             buffer_offset;

    //constructor
    CurlConnection(int id,CurlNetService* owner_) : Connection(id),owner(owner_), handle(nullptr), slist(0), last_size_download(0), last_size_upload(0), buffer_offset(0)
    {
      memset(errbuf, 0, sizeof(errbuf));
      this->handle = curl_easy_init();
    }

    //destructor
    ~CurlConnection()
    {
      if (slist != nullptr) curl_slist_free_all(slist);
      curl_easy_cleanup(handle);
    }

    //setNetJob
    virtual void setNetRequest(NetRequest user_request,Promise<NetResponse> user_promise) override
    {
      if (this->request.valid())
      {
        curl_multi_remove_handle(owner->multi_handle, this->handle);
        curl_easy_reset(this->handle);
      }

      this->request  = user_request;
      this->response = NetResponse();
      this->promise  = user_promise;

      this->buffer_offset = 0;
      this->last_size_download = 0;
      this->last_size_upload = 0;
      memset(errbuf, 0, sizeof(errbuf));

      if (this->request.valid())
      {
        curl_easy_setopt(this->handle, CURLOPT_FORBID_REUSE, 1L); //not sure if this is the best option (see http://www.perlmonks.org/?node_id=925760)
        curl_easy_setopt(this->handle, CURLOPT_FRESH_CONNECT, 1L);
        curl_easy_setopt(this->handle, CURLOPT_NOSIGNAL, 1L); //otherwise crash on linux
        curl_easy_setopt(this->handle, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(this->handle, CURLOPT_VERBOSE, 0L); //SET to 1L if you want to debug !

        String cacertfile=CaCertFile::getSingleton()->getFilename();
        if (!cacertfile.empty())
          curl_easy_setopt(this->handle, CURLOPT_CAINFO, &cacertfile[0]);
        else {
          //VisusInfo()<<"Disabling SSL verify peer since I cannot use any valid CaCertFile, potential security hole";
          curl_easy_setopt(this->handle, CURLOPT_SSL_VERIFYPEER, 0L);
        }

        curl_easy_setopt(this->handle, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(this->handle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(this->handle, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(this->handle, CURLOPT_USERAGENT, "visus/libcurl");
        curl_easy_setopt(this->handle, CURLOPT_ERRORBUFFER, this->errbuf);
        curl_easy_setopt(this->handle, CURLOPT_HEADERFUNCTION, HeaderFunction);
        curl_easy_setopt(this->handle, CURLOPT_WRITEFUNCTION, WriteFunction);
        curl_easy_setopt(this->handle, CURLOPT_READFUNCTION, ReadFunction);
        curl_easy_setopt(this->handle, CURLOPT_PRIVATE, this);
        curl_easy_setopt(this->handle, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(this->handle, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(this->handle, CURLOPT_READDATA, this);

        String proxy = VisusConfig::readString("Configuration/NetService/proxy");
        if (!proxy.empty())
        {
          curl_easy_setopt(this->handle, CURLOPT_PROXY, proxy.c_str());
          if (int proxy_port = cint(VisusConfig::readString("Configuration/NetService/proxyport")))
            curl_easy_setopt(this->handle, CURLOPT_PROXYPORT, proxy_port);
        }

        curl_easy_setopt(this->handle, CURLOPT_URL, request.url.toString().c_str());

        //set request_headers
        if (this->slist != nullptr) curl_slist_free_all(this->slist);
        this->slist = nullptr;
        for (auto it = request.headers.begin(); it != request.headers.end(); it++)
        {
          String temp = it->first + ":" + it->second;
          this->slist = curl_slist_append(this->slist, temp.c_str());
        }

        //see https://www.redhat.com/archives/libvir-list/2012-February/msg00860.html
        //this is needed for example for VisusNetRerver that does not send "100 (Continue)"
        this->slist = curl_slist_append(this->slist, "Expect:");

        if (request.method == "DELETE")
        {
          curl_easy_setopt(this->handle, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        else if (request.method == "GET")
        {
          curl_easy_setopt(this->handle, CURLOPT_HTTPGET, 1);
        }
        else if (request.method == "HEAD")
        {
          curl_easy_setopt(this->handle, CURLOPT_HTTPGET, 1);
          curl_easy_setopt(this->handle, CURLOPT_NOBODY, 1);
        }
        else if (request.method == "POST")
        {
          VisusAssert(false); //TODO: not sure if I need it
        }
        else if (request.method == "PUT")
        {
          curl_easy_setopt(this->handle, CURLOPT_UPLOAD, 1);
          curl_easy_setopt(this->handle, CURLOPT_PUT, 1);
          curl_easy_setopt(this->handle, CURLOPT_INFILESIZE, request.body->c_size());
        }
        else
        {
          VisusAssert(false);
        }

        curl_easy_setopt(this->handle, CURLOPT_HTTPHEADER, this->slist);
        curl_multi_add_handle(owner->multi_handle, this->handle);
      }
    }

    //HeaderFunction
    static size_t HeaderFunction(void *ptr, size_t size, size_t nmemb, CurlConnection *connection)
    {
      connection->first_byte=true;

      if (!connection->response.body)
        connection->response.body = std::make_shared<HeapMemory>();

      size_t tot = size*nmemb;
      char* p = strchr((char*)ptr, ':');
      if (p)
      {
        String key = StringUtils::trim(String((char*)ptr, p));
        String value = StringUtils::trim(String((char*)p + 1, (char*)ptr + tot));
        if (!key.empty()) connection->response.setHeader(key, value);

        //avoid too much overhead for writeFunction function
        if (StringUtils::toLower(key) == "content-length")
        {
          int content_length = cint(value);
          connection->response.body->reserve(content_length, __FILE__, __LINE__);
        }

      }
      return(nmemb*size);
    }

    //WriteFunction
    static size_t WriteFunction(void *chunk, size_t size, size_t nmemb, CurlConnection *connection)
    {
      connection->first_byte=true;

      if (!connection->response.body)
        connection->response.body = std::make_shared<HeapMemory>();

      size_t tot = size * nmemb;
      Int64 oldsize = connection->response.body->c_size();
      if (!connection->response.body->resize(oldsize + tot, __FILE__, __LINE__))
      {
        VisusAssert(false); return 0;
      }
      memcpy(connection->response.body->c_ptr() + oldsize, chunk, tot);
      ApplicationStats::net.trackReadOperation(tot);
      return tot;
    }

    //ReadFunction
    static size_t ReadFunction(char *chunk, size_t size, size_t nmemb, CurlConnection *connection)
    {
      connection->first_byte=true;

      size_t& offset = connection->buffer_offset;
      size_t tot = std::min((size_t)connection->request.body->c_size() - offset, size * nmemb);
      memcpy(chunk, connection->request.body->c_ptr() + offset, tot);
      offset += tot;
      ApplicationStats::net.trackWriteOperation(tot);
      return tot;
    }

  };

  NetService* owner;
  CURLM*  multi_handle = nullptr;

  //constructor
  CurlNetService(NetService* owner_) : owner(owner_) {
  }

  //destructor
  ~CurlNetService()
  {
    if (multi_handle)
      curl_multi_cleanup(multi_handle);
  }

  //createConnection
  virtual SharedPtr<Connection> createConnection(int id) override  {

    //important to create in this thread
    if (!multi_handle)
      multi_handle=curl_multi_init();

    return std::make_shared<CurlConnection>(id,this);
  }

  //runMore
  virtual void runMore(const std::set<Connection*>& running) override
  {
    if (running.empty())
      return;

    for (int multi_perform = CURLM_CALL_MULTI_PERFORM; multi_perform == CURLM_CALL_MULTI_PERFORM;)
    {
      int running_handles_;
      multi_perform = curl_multi_perform(multi_handle, &running_handles_);

      CURLMsg *msg; int msgs_left_;
      while ((msg = curl_multi_info_read(multi_handle, &msgs_left_)))
      {
        CurlConnection* connection = nullptr;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &connection); VisusAssert(connection);

        if (msg->msg == CURLMSG_DONE)
        {
          long response_code = 0;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &response_code);

          //in case the request fails before being sent, the response_code is zero. 
          connection->response.status = response_code? response_code : HttpStatus::STATUS_BAD_REQUEST; 

          if (msg->data.result != CURLE_OK)
            connection->response.setErrorMessage(String(connection->errbuf));
        }
      }
    }
  }

};


/////////////////////////////////////////////////////////////////////////////
NetService::NetService()
{
}

/////////////////////////////////////////////////////////////////////////////
NetService::~NetService()
{
  if (this->pimpl)
  {
    asyncNetworkIO(SharedPtr<NetRequest>()); //fake request to exit from the thread
    Thread::join(thread);
    thread.reset();

    delete pimpl;
  }
}

/////////////////////////////////////////////////////////////////////////////
void NetService::startNetService()
{
  VisusAssert(!pimpl);

  if (type == NetSocket::Bsd)
    this->pimpl = new CurlNetService(this);
  else
    ThrowException("internal error");

  thread=Thread::start("Net Service Thread",[this](){
    entryProc();
  });
}

/////////////////////////////////////////////////////////////////////////////
Future<NetResponse> NetService::asyncNetworkIO(SharedPtr<NetRequest> request)
{
  if (request)
    request->statistics.enter_t1=Time::now();

  ApplicationStats::num_net_jobs++;

  Promise<NetResponse> promise;
  {
    ScopedLock lock(waiting_lock);
    waiting.push_back(std::make_pair(request,promise));
  }
  got_request.up();
  return promise.get_future();
}

/////////////////////////////////////////////////////////////////////////////
void NetService::printStatistics(int connection_id,const NetRequest& request,const NetResponse& response)
{
  std::ostringstream out;
  out << request.method;
  out<<" connection("<<connection_id<<")";

  out << " wait(" << (request.statistics.wait_msec) << ") running(" << (request.statistics.run_msec) << ")";

  if (request.method == "GET")
  {
    Int64 overall = response.body? response.body->c_size() :0;
    double speed=overall/(request.statistics.run_msec/1000.0);
    out << " download(" << StringUtils::getStringFromByteSize(overall) << " - " << (int)(speed / 1024) << "kb/sec)";
  }
  else if (request.method == "PUT")
  {
    Int64 overall = request.body? request.body->c_size() : 0;
    double speed=overall/(request.statistics.run_msec/1000.0);
    out << " updload(" << StringUtils::getStringFromByteSize(overall) << " - " << (int)(speed / 1024) << "kb/sec)";
  }

  out<<" status("<<response.getStatusDescription()<<")";
  out<<" url("<<request.url.toString()<<")";

  VisusInfo() << out.str();
}


/////////////////////////////////////////////////////////////////////////////
void NetService::entryProc() 
{
  std::vector< SharedPtr<Connection> > connections;
  for (int I = 0; I < nconnections; I++)
    connections.push_back(pimpl->createConnection(I));
  VisusAssert(!connections.empty());

  std::list<Connection*> available;
  for (auto connection : connections)
    available.push_back(connection.get());

  std::set<Connection*> running;
  std::deque<Int64> last_sec_connections;
  bool bExitThread=false;
  while (true)
  {
    //finished?
    {
      ScopedLock lock(waiting_lock);
      while (waiting.empty() && running.empty())
      {
        if (bExitThread)
          return;
 
        this->waiting_lock.unlock();
        got_request.down();
        this->waiting_lock.lock();
      }
    }

    //handle waiting
    {
      ScopedLock lock(waiting_lock);
      Waiting still_waiting;
      for (auto it : waiting)
      {
        auto request = it.first;
        auto promise = it.second;

        //request to exit ASAP (no need to execute it)
        if (!request) 
        {
          ApplicationStats::num_net_jobs--;

          bExitThread=true;
          continue;
        }

        //was aborted
        if (request->aborted() || bExitThread)
        {
          request->statistics.wait_msec = (int)request->statistics.enter_t1.elapsedMsec();
          this->waiting_lock.unlock();
          {
            auto response=NetResponse(HttpStatus::STATUS_SERVICE_UNAVAILABLE);

            request->statistics.run_msec=0;
            if (verbose > 0)
              printStatistics(-1,*request,response);
            promise.set_value(response);

            ApplicationStats::num_net_jobs--;
          }
          this->waiting_lock.lock();
          continue;
        }

        if (available.empty()) 
        {
          still_waiting.push_back(it);
          continue;
        }

        //there is the max_connection_per_sec to respect!
        if (max_connections_per_sec)
        {
          Int64 now_timestamp = Time::getTimeStamp();

          //purge too old
          while (!last_sec_connections.empty() && (now_timestamp - last_sec_connections.front()) > 1000)
            last_sec_connections.pop_front();

          if ((int)last_sec_connections.size() >= max_connections_per_sec)
          {
            still_waiting.push_back(it);
            continue;
          }

          last_sec_connections.push_back(now_timestamp); ///
        }

        //don't start connection too soon, they can be soon aborted
        int wait_msec = (int)request->statistics.enter_t1.elapsedMsec();
        if (wait_msec < min_wait_time)
        {
          still_waiting.push_back(it);
          continue;
        }
 
        //wait -> running
        Connection* connection = available.front();
        available.pop_front();
        running.insert(connection);

        request->statistics.wait_msec = wait_msec;
        request->statistics.run_t1 = Time::now();
        connection->first_byte = false;
        connection->setNetRequest(*request, promise);
        ApplicationStats::net.trackOpen(); 
      }
      waiting=still_waiting;
    }

    if (!running.empty())
    {
      pimpl->runMore(running);

      for (auto connection : std::set<Connection*>(running))
      {
        //run->done (since aborted)
        if (connection->request.aborted() || bExitThread)
          connection->response=NetResponse(HttpStatus::STATUS_SERVICE_UNAVAILABLE);

        //timeout (i.e. didn' receive first byte after a certain amount of seconds
        else if (!connection->first_byte && connect_timeout>0 && connection->request.statistics.run_msec>=(this->connect_timeout*1000))
          connection->response=NetResponse(HttpStatus::STATUS_REQUEST_TIMEOUT);

        //still running
        if (!connection->response.status) 
          continue;

        connection->request.statistics.run_msec = (int)connection->request.statistics.run_t1.elapsedMsec();
        if (verbose > 0)
          printStatistics(connection->id, connection->request,connection->response);
        connection->promise.set_value(connection->response);

        connection->setNetRequest(NetRequest(), Promise<NetResponse>());
        running.erase(connection);
        available.push_back(connection);
        ApplicationStats::num_net_jobs--;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////
NetResponse NetService::getNetResponse(NetRequest request)
{
  auto netservice=std::make_shared<NetService>();
  netservice->setNumberOfConnections(1);
  netservice->setSocketType(NetSocket::guessTypeFromProtocol(request.url.getProtocol()));
  netservice->startNetService();

  NetResponse response=netservice->asyncNetworkIO(request).get();

  if (!response.isSuccessful() && !request.aborted())
    VisusWarning()<<"request "<<request.url.toString()<<" failed ("<<response.getErrorMessage()<<")";

  return response;
}


} //namespace Visus


