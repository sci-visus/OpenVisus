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

#include <Visus/NetSocket.h>
#include <Visus/VisusConfig.h>
#include <Visus/Log.h>


#if WIN32
#pragma warning(disable:4996)
#include <WinSock2.h>
#define getIpCat(cat)    htonl(cat)
typedef int socklen_t;
#define SHUT_RD   SD_RECEIVE 
#define SHUT_WR   SD_SEND 
#define SHUT_RDWR SD_BOTH 
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <unistd.h>
#define getIpCat(cat)          cat
#define closesocket(socketref) ::close(socketref)
#endif 

namespace Visus {

//////////////////////////////////////////////////////////////
class BSDNetSocketPimpl : public NetSocket::Pimpl
{
public:

  //constructor
  BSDNetSocketPimpl() : socketfd(-1)
  {}

  //destructor
  ~BSDNetSocketPimpl()
  { 
    if (socketfd>=0)
      closesocket(socketfd);
  }

  //getNativeHandle
  virtual void* getNativeHandle() override
  {return &socketfd;}

  //close
  virtual void close() override
  {
    if (socketfd<0) return;
    closesocket(socketfd);
    socketfd = -1;
  }

  //shutdownSend
  virtual void shutdownSend() override
  {
    if (socketfd<0) return;
    ::shutdown(socketfd, SHUT_WR);
  }

  //connect
  virtual bool connect(String url_) override
  {
    Url url(url_);

    close();

    this->socketfd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (this->socketfd < 0)
    {
      VisusError() << "connect failed, reason socket(AF_INET, SOCK_STREAM, 0) returned <0 (" << strerror(errno) << ")";
      return false;
    }

    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
  
    serv_addr.sin_addr.s_addr = getIPAddress(url.getHostname().c_str());

    configureOptions("Configuration/NetSocket/connect");

    serv_addr.sin_port = htons(url.getPort());
    if (::connect(this->socketfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
      VisusError() << "connect failed, cannot connect (" << strerror(errno) << ")";
      return false;
    }

    return true;
  }

  //bind
  virtual bool bind(String url_) override
  {
    close();

    Url url(url_);
    VisusAssert(url.getHostname()=="*" || url.getHostname()=="localhost" || url.getHostname()=="127.0.0.1");

    this->socketfd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (this->socketfd<0)
    {
      VisusError() << "bind failed (socketfd<0) a server-side socket (" << strerror(errno) << ")";
      return false;
    }

    if (bool bReuseAddress=true)
    {
      const int reuse_addr = 1;
      setsockopt(this->socketfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr));
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons((unsigned short)url.getPort());
    sin.sin_addr.s_addr = getIpCat(INADDR_ANY);
    if (::bind(this->socketfd, (struct sockaddr*)(&sin), sizeof(struct sockaddr)))
    {
      close();
      VisusError() << "bind failed. can't bind for server-side socket (" << strerror(errno) << ")";
      return false;
    }

    const int max_connections = SOMAXCONN;
    this->configureOptions("Configuration/NetSocket/listen");
    if (::listen(this->socketfd, max_connections))
    {
      close();
      VisusError() << "listen failed. Can't listen (listen(...) method) for server-side socket (" << strerror(errno) << ")";
      return false;
    }

    VisusInfo() << "NetSocket::bind ok url("<<url.toString()<<")";
    return true;
  }

  //acceptConnection
  virtual SharedPtr<NetSocket> acceptConnection() override
  {
    if (socketfd<0) 
      return SharedPtr<NetSocket>();

    UniquePtr<BSDNetSocketPimpl> pimpl(new BSDNetSocketPimpl());

    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    pimpl->socketfd = (int)::accept(this->socketfd, (struct sockaddr*)(&client_addr), &sin_size);
    if (pimpl->socketfd < 0)
    {
      VisusError() << "accept failed. (" << strerror(errno) << ")";
      return SharedPtr<NetSocket>();
    }

    pimpl->configureOptions("Configuration/NetSocket/accept");

    VisusInfo() << "NetSocket accepted new connection";
    return std::make_shared<NetSocket>(NetSocket::Bsd,pimpl.release());
  }

  //sendRequest
  virtual bool sendRequest(NetRequest request) override
  {
    String headers=request.getHeadersAsString();

    if (!sendBytes((const Uint8*)headers.c_str(),(int)headers.size()))
      return false;

    if (request.body && request.body->c_size())
    {
      VisusAssert(request.getContentLength()==request.body->c_size());
      if (!sendBytes(request.body->c_ptr(),(int)request.body->c_size()))
        return false;
    }

    return true;
  }

  //sendResponse
  virtual bool sendResponse(NetResponse response) override
  {
    String headers=response.getHeadersAsString();

    if (!sendBytes((const Uint8*)headers.c_str(),(int)headers.size()))
      return false;

    if (response.body && response.body->c_size())
    {
      VisusAssert(response.getContentLength()==response.body->c_size());
      if (!sendBytes(response.body->c_ptr(),(int)response.body->c_size()))
        return false;
    }

    return true;
  }

  //receiveRequest
  virtual NetRequest receiveRequest() override
  {
    String headers;
    headers.reserve(8192);
    while (!StringUtils::endsWith(headers, "\r\n\r\n"))
    {
      if (headers.capacity() == headers.size())
        headers.reserve(headers.capacity() << 1);

      char ch = 0;
      if (!receiveBytes((unsigned char*)&ch, 1))
        return NetRequest();

      headers.push_back(ch);
    }

    NetRequest request;
    if (!request.setHeadersFromString(headers))
      return NetRequest();
    
    if (int ContentLength=(int)request.getContentLength())
    {
      request.body=std::make_shared<HeapMemory>();
      if (!request.body->resize(ContentLength,__FILE__,__LINE__))
        return NetRequest();

      if (!receiveBytes(request.body->c_ptr(), (int)ContentLength))
        return NetRequest();
    }

    return request;
  }

  //receiveResponse
  virtual NetResponse receiveResponse() override
  {
    String headers;
    headers.reserve(8192);
    while (!StringUtils::endsWith(headers, "\r\n\r\n"))
    {
      if (headers.capacity() == headers.size())
        headers.reserve(headers.capacity() << 1);

      char ch = 0;
      if (!receiveBytes((unsigned char*)&ch, 1))
        return NetResponse();

      headers.push_back(ch);
    }

    NetResponse response;
    if (!response.setHeadersFromString(headers))
      return NetResponse();
    
    if (int ContentLength=(int)response.getContentLength())
    {
      response.body=std::make_shared<HeapMemory>();
      if (!response.body->resize(ContentLength,__FILE__,__LINE__))
        return NetResponse();

      if (!receiveBytes(response.body->c_ptr(), (int)ContentLength))
        return NetResponse();
    }

    return response;
  }

private:

  VISUS_NON_COPYABLE_CLASS(BSDNetSocketPimpl)

  int socketfd;

  //configureOptions
  void configureOptions(String config_key)
  {
    int send_buffer_size = cint(VisusConfig::readString(config_key+"/send_buffer_size"));
    if (send_buffer_size > 0) 
      setSendBufferSize(send_buffer_size);

    int recv_buffer_size = cint(VisusConfig::readString(config_key+"/recv_buffer_size"));
    if (recv_buffer_size > 0) 
      setReceiveBufferSize(recv_buffer_size);

    //I think the no delay should be always enabled in Visus
    bool tcp_no_delay = cbool(VisusConfig::readString(config_key+"/tcp_no_delay","1"));
    if (tcp_no_delay) 
      setNoDelay(tcp_no_delay);
  }

  //setNoDelay
  void setNoDelay(bool bValue)
  {
    int value=bValue?1:0;
    setsockopt(this->socketfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof(value));
  }

  //setSendBufferSize
  void setSendBufferSize(int value)
  {
    setsockopt(this->socketfd, SOL_SOCKET, SO_SNDBUF, (const char*)&value, sizeof(value));
  }

  //setReceiveBufferSize
  void setReceiveBufferSize(int value)
  {
    setsockopt(this->socketfd, SOL_SOCKET, SO_RCVBUF, (const char*)&value, sizeof(value));
  }

  //sendBytes
  bool sendBytes(const unsigned char *buf, int len)
  {
    if (socketfd<0) 
      return false;

    int flags=0;
  
    while (len)
    {
      int n = (int)::send(socketfd, (const char*)buf, len, flags);
      if (n <= 0)
      {
        VisusError() << "Failed to send data to socket errdescr(" << getSocketErrorDescription(n) << ")";
        return false;
      }
      buf += n;
      len -= n;
    }
    return true;
  }

  //receiveBytes
  bool receiveBytes(unsigned char *buf, int len)
  {
    if (socketfd<0) 
      return false;

    int flags=0;

    while (len)
    {
      int n = (int)::recv(socketfd, (char*)buf, len, flags);
      if (n <= 0)
      {
        VisusError() << "Failed to recv data to socket errdescr(" << getSocketErrorDescription(n) << ")";
        return false;
      }
      buf += n;
      len -= n;
    }
    return true;
  }

  //getSocketErrorDescription
  static inline const char* getSocketErrorDescription(int retcode)
  {
    switch (retcode)
    {
    case EACCES:return "EACCES";
    case EAGAIN:return "EAGAIN";
    #if EWOULDBLOCK!=EAGAIN
    case EWOULDBLOCK:return "EWOULDBLOCK";
    #endif
    case EBADF:return "EBADF";
    case ECONNRESET:return "ECONNRESET";
    case EDESTADDRREQ:return "EDESTADDRREQ";
    case EFAULT:return "EFAULT";
    case EINTR:return "EINTR";
    case EINVAL:return "EINVAL";
    case EISCONN:return "EISCONN";
    case EMSGSIZE:return "EMSGSIZE";
    case ENOBUFS:return "ENOBUFS";
    case ENOMEM:return "ENOMEM";
    case ENOTCONN:return "ENOTCONN";
    case ENOTSOCK:return "ENOTSOCK";
    case EOPNOTSUPP:return "EOPNOTSUPP";
    case EPIPE:return "EPIPE";
    case ECONNREFUSED: return "ECONNREFUSED";
    case EDOM: return "EDOM";
    case ENOPROTOOPT: return "ENOPROTOOPT";
    }
    return "Unknown";
  }

  //getIPAddress
  static inline unsigned long getIPAddress(const char* pcHost)
  {
    u_long nRemoteAddr = inet_addr(pcHost);
    if (nRemoteAddr == INADDR_NONE)
    {
      hostent* pHE = gethostbyname(pcHost);
      if (pHE == 0) return INADDR_NONE;
      nRemoteAddr = *((u_long*)pHE->h_addr_list[0]);
    }
    return nRemoteAddr;
  }

};

////////////////////////////////////////////////////////////////////
NetSocket::NetSocket(Type type_,SharedPtr<NetSocket> share_context_with) : type(type_)
{
  if (type==Bsd)
  {
    VisusAssert(!share_context_with);
    this->pimpl=new BSDNetSocketPimpl();
  }
  else
  {
    ThrowException("internal error");
  }
}


} //namespace Visus

