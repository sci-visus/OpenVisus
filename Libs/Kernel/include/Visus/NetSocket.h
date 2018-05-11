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

#ifndef VISUS_NET_SOCKET_H
#define VISUS_NET_SOCKET_H

#include <Visus/Kernel.h>
#include <Visus/NetMessage.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API NetSocket
{
public:

  VISUS_NON_COPYABLE_CLASS(NetSocket)

  //______________________________________________
  class Pimpl
  {
  public:

    //constructors
    Pimpl()
    {}

    //destructor
    virtual ~Pimpl()
    {}

    //getNativeHandle
    virtual void* getNativeHandle()=0;

    //shutdownSend
    virtual void shutdownSend()=0;

    //close
    virtual void close()=0;

    //connect (client side)
    virtual bool connect(String url)=0;

    //bind (server side)
    virtual bool bind(String url)=0;

    //acceptConnection (server side)
    virtual SharedPtr<NetSocket> acceptConnection()=0;

    //sendRequest
    virtual bool sendRequest(NetRequest request)=0;

    //sendResponse
    virtual bool sendResponse(NetResponse response)=0;

    //receiveRequest
    virtual NetRequest receiveRequest()=0;

    //receiveResponse
    virtual NetResponse receiveResponse()=0;
  
  };

  //constructor
  NetSocket();

  //constructor
  NetSocket(Pimpl* VISUS_DISOWN(pimpl_)) : pimpl(pimpl_){
  }

  //destructor
  virtual ~NetSocket() {
    if (pimpl) delete pimpl;
  }

  //getPimpl
  virtual Pimpl* getPimpl() const {
    return pimpl;
  }

  //shutdownSend
  void shutdownSend() {
    return pimpl->shutdownSend();
  }

  //close
  void close() {
    return pimpl->close();
  }

  //connect (client side)
  bool connect(String url) {
    return pimpl->connect(url);
  }

  //bind (server side)
  bool bind(String url) {
    return pimpl->bind(url);
  }

  //acceptConnection (server side)
  SharedPtr<NetSocket> acceptConnection() {
    return pimpl->acceptConnection();
  }

  //sendRequest
  bool sendRequest(NetRequest request) {
    return pimpl->sendRequest(request);
  }

  //sendResponse
  bool sendResponse(NetResponse response) {
    return pimpl->sendResponse(response);
  }

  //receiveRequest
  NetRequest receiveRequest() {
    return pimpl->receiveRequest();
  }

  //receiveResponse
  NetResponse receiveResponse() {
    return pimpl->receiveResponse();
  }

private:

  Pimpl* pimpl;

};


} //namespace Visus


#endif //VISUS_NET_SOCKET_H
