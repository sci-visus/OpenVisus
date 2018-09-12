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

#ifndef VISUS_NETWORK_SERVICE_H
#define VISUS_NETWORK_SERVICE_H

#include <Visus/Kernel.h>
#include <Visus/NetMessage.h>
#include <Visus/Async.h>
#include <Visus/NetSocket.h>

#include <thread>

namespace Visus {


  ///////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API CaCertFile
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(CaCertFile)

    //getFilename
    const String& getFilename() const {
    return local_filename;
  }

private:

  String local_filename;

  //constructor
  CaCertFile();

};


///////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API NetService 
{
public:

  VISUS_NON_COPYABLE_CLASS(NetService)

  //_________________________________________________________
  class VISUS_KERNEL_API Connection
  {
  public:

    VISUS_CLASS(Connection)

    int                              id=0;
    NetRequest                       request;
    Promise<NetResponse>             promise;
    NetResponse                      response;
    bool                             first_byte=false;

    //constructor
    Connection(int id_) : id(id_) {
    }

    //destructor
    virtual ~Connection() {
    }

    //setNetJob
    virtual void setNetRequest(NetRequest request,Promise<NetResponse> promise) = 0;

  };

  //_________________________________________________________
  class VISUS_KERNEL_API Pimpl 
  {
  public:
    
    //destrutor
    virtual ~Pimpl(){
    }

    //runMore
    virtual void runMore(const std::set<Connection*>& running) = 0;

    //createConnection
    virtual SharedPtr<Connection> createConnection(int id) = 0;

  };

  //constructor
  NetService(int nconnections,bool bVerbose=1);

  //destructor
  virtual ~NetService();

  //setSocketType
  void setVerbose(int value) {
    VisusAssert(!pimpl);
    this->verbose=value;
  }

  //getConnectTimeout
  int getConnectTimeout() const {
    return connect_timeout;
  }

  //setConnectTimeout
  void setConnectTimeout(int value) {
    VisusAssert(!pimpl);
    this->connect_timeout=value;
  }

  //push
  static Future<NetResponse> push(SharedPtr<NetService> service, NetRequest request);

  //getNetResponse
  static NetResponse getNetResponse(NetRequest request);


private:

  typedef std::list< std::pair< SharedPtr<NetRequest> , Promise<NetResponse> > > Waiting;

  int                          nconnections = 8;
  int                          min_wait_time = 10;
  int                          max_connections_per_sec = 0;
  int                          connect_timeout = 10; //in seconds (explanation in CONNECTTIMEOUT)
  int                          verbose = 1;

  CriticalSection              waiting_lock;
  Waiting                      waiting;

  SharedPtr<std::thread>       thread;
  Semaphore                    got_request;

  Pimpl*                       pimpl=nullptr;

  //entryProc
  void entryProc();

  //printStatistics
  void printStatistics(int connection_id,const NetRequest& request,const NetResponse& response);

  //handleAsync
  Future<NetResponse> handleAsync(SharedPtr<NetRequest> request);

};


} //namespace Visus


#endif //VISUS_NETWORK_SERVICE_H




