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

#ifndef VISUS_AMAZON_CLOUD_STORAGE_
#define VISUS_AMAZON_CLOUD_STORAGE_

#include <Visus/Kernel.h>
#include <Visus/CloudStorage.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////
/*
https://docs.aws.amazon.com/AmazonS3/latest/user-guide/using-folders.html
*/
class AmazonCloudStorage : public CloudStorage
{
public:

  VISUS_CLASS(AmazonCloudStorage)

    String protocol;
  String hostname;
  String username;
  String password;

  String container;

  //constructor
  AmazonCloudStorage(Url url)
  {
    this->username = url.getParam("username");
    VisusAssert(!this->username.empty());

    this->password = url.getParam("password");
    VisusAssert(!this->password.empty());
    this->password = StringUtils::base64Decode(password);

    this->protocol = url.getProtocol();
    this->hostname = url.getHostname();
  }

  //destructor
  virtual ~AmazonCloudStorage() {
  }

  //signRequest
  void signRequest(NetRequest& request)
  {
    String bucket = StringUtils::split(request.url.getHostname(), ".")[0];
    VisusAssert(!bucket.empty());

    String canonicalized_resource = "/" + bucket + request.url.getPath();

    String canonicalized_headers;
    {
      std::ostringstream out;
      for (auto it = request.headers.begin(); it != request.headers.end(); it++)
      {
        if (StringUtils::startsWith(it->first, "x-amz-"))
          out << StringUtils::toLower(it->first) << ":" << it->second << "\n";
      }
      canonicalized_headers = out.str();
    }

    char date_GTM[256];
    time_t t; time(&t);
    struct tm *ptm = gmtime(&t);
    strftime(date_GTM, sizeof(date_GTM), "%a, %d %b %Y %H:%M:%S GMT", ptm);

    String signature = request.method + "\n";
    signature += request.getHeader("Content-MD5") + "\n";
    signature += request.getContentType() + "\n";
    signature += String(date_GTM) + "\n";
    signature += canonicalized_headers;
    signature += canonicalized_resource;
    signature = StringUtils::base64Encode(StringUtils::hmac_sha1(signature, password));
    request.setHeader("Host", request.url.getHostname());
    request.setHeader("Date", date_GTM);
    request.setHeader("Authorization", "AWS " + username + ":" + signature);
  }

  // addContainer
  Future<bool> addContainer(SharedPtr<NetService> service, String container, Aborted aborted = Aborted())
  {
    VisusAssert(!StringUtils::contains(container, "/"));

    auto ret = Promise<bool>().get_future();

    //I know that the container already exists, don't need to create it
    if (container == this->container)
    {
      ret.get_promise()->set_value(true);
      return ret;
    }

    NetRequest request(this->protocol + "://" + this->hostname + "/" + container, "PUT");
    request.aborted = aborted;
    request.url.setPath(request.url.getPath() + "/"); //IMPORTANT the "/" to indicate is a container! see http://www.bucketexplorer.com/documentation/amazon-s3--how-to-create-a-folder.html
    signRequest(request);

    NetService::push(service, request).when_ready([this, ret, container](NetResponse response) {
      bool bOk = response.isSuccessful();
      if (bOk)
        this->container = container;
      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // deleteContainer
  Future<bool> deleteContainer(SharedPtr<NetService> service, String container_name, Aborted aborted = Aborted())
  {
    VisusAssert(!StringUtils::contains(container_name, "/"));

    auto ret = Promise<bool>().get_future();

    NetRequest request(this->protocol + "://" + this->hostname + "/" + container_name, "DELETE");
    request.aborted = aborted;
    request.url.setPath(request.url.getPath() + "/"); //IMPORTANT the "/" to indicate is a container!
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }

  // addBlob 
  virtual Future<bool> addBlob(SharedPtr<NetService> service, String blob_name, Blob blob, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<bool>().get_future();

    //example /container_name/aaa/bbb/filename.pdf
    VisusAssert(StringUtils::startsWith(blob_name, "/"));
    auto v = StringUtils::split(blob_name, "/",/*bPurgeEmptyItems*/true);
    VisusAssert(v.size() >= 2);
    String container = v[0];

    addContainer(service, container, aborted).when_ready([this, ret, service, blob, blob_name, aborted](bool bOk)
    {
      if (!bOk)
      {
        ret.get_promise()->set_value(false);
        return;
      }

      //NOTE blob_name already contains the container name
      NetRequest request(this->protocol + "://" + this->hostname + blob_name, "PUT");
      request.aborted = aborted;
      request.body = blob.body;
      request.setContentLength(blob.body->c_size());
      request.setContentType(blob.content_type);

      //metadata
      for (auto it : blob.metadata)
        request.setHeader("x-amz-meta-" + it.first, it.second);

      signRequest(request);

      NetService::push(service, request).when_ready([ret](NetResponse response) {
        bool bOk = response.isSuccessful();
        ret.get_promise()->set_value(bOk);
      });
    });

    return ret;
  }

  // getBlob 
  virtual Future<Blob> getBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<Blob>().get_future();

    //NOTE blob_name already contains the container name
    NetRequest request(this->protocol + "://" + this->hostname + blob_name, "GET");
    request.aborted = aborted;
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {

      Blob blob;

      if (response.isSuccessful())
      {
        //parse metadata
        String metatata_prefix = "x-amz-meta-";
        for (auto it = response.headers.begin(); it != response.headers.end(); it++)
        {
          String name = it->first;
          if (StringUtils::startsWith(name, metatata_prefix))
          {
            name = StringUtils::replaceAll(name.substr(metatata_prefix.length()), "_", "-"); //backward compatibility
            blob.metadata.setValue(name, it->second);
          }
        }

        blob.body = response.body;

        auto content_type = response.getContentType();
        if (!content_type.empty())
          blob.content_type = content_type;
      }

      ret.get_promise()->set_value(blob);
    });

    return ret;
  }

  // deleteBlob
  virtual Future<bool> deleteBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    //NOTE blob_name already contains the container name
    NetRequest request(this->protocol + "://" + this->hostname + blob_name, "DELETE");
    request.aborted = aborted;
    signRequest(request);

    auto ret = Promise<bool>().get_future();

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }

};

}//namespace

#endif //VISUS_AMAZON_CLOUD_STORAGE_

