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

#ifndef VISUS_AZURE_CLOUD_STORAGE_
#define VISUS_AZURE_CLOUD_STORAGE_

#include <Visus/Kernel.h>
#include <Visus/CloudStorage.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////// 
class AzureCloudStorage : public CloudStorage
{
public:

  Url    url;
  String account_name;
  String access_key;

  String container;

  //constructor
  AzureCloudStorage(Url url)
  {
    this->access_key = url.getParam("access_key");
    VisusAssert(!access_key.empty());
    this->access_key = StringUtils::base64Decode(access_key);

    this->account_name = url.getHostname().substr(0, url.getHostname().find('.'));

    this->url = url.getProtocol() + "://" + url.getHostname();
  }

  //destructor
  virtual ~AzureCloudStorage() {
  }

  //signRequest
  void signRequest(NetRequest& request)
  {
    String canonicalized_resource = "/" + this->account_name + request.url.getPath();

    if (!request.url.params.empty())
    {
      std::ostringstream out;
      for (auto it = request.url.params.begin(); it != request.url.params.end(); ++it)
        out << "\n" << it->first << ":" << it->second;
      canonicalized_resource += out.str();
    }

    char date_GTM[256];
    time_t t; time(&t);
    struct tm *ptm = gmtime(&t);
    strftime(date_GTM, sizeof(date_GTM), "%a, %d %b %Y %H:%M:%S GMT", ptm);

    request.setHeader("x-ms-version", "2018-03-28");
    request.setHeader("x-ms-date", date_GTM);

    String canonicalized_headers;
    {
      std::ostringstream out;
      int N = 0; for (auto it = request.headers.begin(); it != request.headers.end(); it++)
      {
        if (StringUtils::startsWith(it->first, "x-ms-"))
          out << (N++ ? "\n" : "") << StringUtils::toLower(it->first) << ":" << it->second;
      }
      canonicalized_headers = out.str();
    }

    /*
    In the current version, the Content-Length field must be an empty string if the content length of the request is zero.
    In version 2014-02-14 and earlier, the content length was included even if zero.
    See below for more information on the old behavior
    */
    String content_length = request.getHeader("Content-Length");
    if (cint(content_length) == 0)
      content_length = "";

    String signature;
    signature += request.method + "\n";// Verb
    signature += request.getHeader("Content-Encoding") + "\n";
    signature += request.getHeader("Content-Language") + "\n";
    signature += content_length + "\n";
    signature += request.getHeader("Content-MD5") + "\n";
    signature += request.getHeader("Content-Type") + "\n";
    signature += request.getHeader("Date") + "\n";
    signature += request.getHeader("If-Modified-Since") + "\n";
    signature += request.getHeader("If-Match") + "\n";
    signature += request.getHeader("If-None-Match") + "\n";
    signature += request.getHeader("If-Unmodified-Since") + "\n";
    signature += request.getHeader("Range") + "\n";
    signature += canonicalized_headers + "\n";
    signature += canonicalized_resource;

    //if something wrong happens open a "telnet hostname 80", copy and paste what's the request made by curl (setting  CURLOPT_VERBOSE to 1)
    //and compare what azure is signing from what you are using
    //PrintInfo(signature);

    signature = StringUtils::base64Encode(StringUtils::hmac_sha256(signature, this->access_key));

    request.setHeader("Authorization", "SharedKey " + account_name + ":" + signature);
  }

  //setContainer
  Future<bool> addContainer(SharedPtr<NetService> service, String container, Aborted aborted = Aborted())
  {
    VisusAssert(!StringUtils::contains(container, "/"));

    auto ret = Promise<bool>().get_future();

    //I know it exists
    if (container == this->container)
    {
      ret.get_promise()->set_value(true);
      return ret;
    }

    NetRequest request(this->url.toString() + "/" + container, "PUT");
    request.aborted = aborted;
    request.url.params.setValue("restype", "container");
    request.setContentLength(0);
    //request.setHeader("x-ms-prop-publicaccess", "container"); IF YOU WANT PUBLIC
    signRequest(request);

    NetService::push(service, request).when_ready([this, ret, container](NetResponse response) {

      bool bOk = response.isSuccessful() || /*bAlreadyExists*/(response.status == 409);
      if (bOk)
        this->container = container;
      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }


  // deleteContainer
  Future<bool> deleteContainer(SharedPtr<NetService> service, String container, Aborted aborted = Aborted())
  {
    VisusAssert(!StringUtils::contains(container, "/"));
    NetRequest request(this->url.toString() + "/" + container, "DELETE");
    request.aborted = aborted;
    request.url.params.setValue("restype", "container");
    signRequest(request);

    auto ret = Promise<bool>().get_future();

    NetService::push(service, request).when_ready([this, container, ret](NetResponse response) {
      bool bOk = response.isSuccessful();
      if (bOk && container == this->container)
        this->container = "";
      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // addBlob
  virtual Future<bool> addBlob(SharedPtr<NetService> service, String blob_name, CloudStorageBlob blob, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<bool>().get_future();

    auto index = blob_name.find("/");
    if (index == String::npos) {
      VisusAssert(false);
      ret.get_promise()->set_value(false);
      return ret;
    }

    String container = blob_name.substr(0, index);

    addContainer(service, container, aborted).when_ready([this, ret, service, blob, blob_name, aborted](bool bOk)
    {
      if (!bOk)
      {
        ret.get_promise()->set_value(false);
        return;
      }

      NetRequest request(this->url.toString() + blob_name, "PUT");
      request.aborted = aborted;
      request.body = blob.body;
      request.setContentLength(blob.body->c_size());
      request.setHeader("x-ms-blob-type", "BlockBlob");
      request.setContentType(blob.content_type);

      for (auto it : blob.metadata)
      {
        auto name = it.first;
        auto value = it.second;

        //name must be a C# variable name
        VisusAssert(!StringUtils::contains(name, "_"));
        if (StringUtils::contains(name, "-"))
          name = StringUtils::replaceAll(name, "-", "_");

        request.setHeader("x-ms-meta-" + name, value);
      }

      signRequest(request);

      NetService::push(service, request).when_ready([ret](NetResponse response) {
        ret.get_promise()->set_value(response.isSuccessful());
      });

    });

    return ret;
  }

  // getBlob 
  virtual Future<CloudStorageBlob> getBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<CloudStorageBlob>().get_future();

    NetRequest request(this->url.toString() + blob_name, "GET");
    request.aborted = aborted;
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {

      if (!response.isSuccessful())
      {
        ret.get_promise()->set_value(CloudStorageBlob());
        return;
      }

      //parse metadata
      CloudStorageBlob blob;
      String metatata_prefix = "x-ms-meta-";
      for (auto it = response.headers.begin(); it != response.headers.end(); it++)
      {
        String name = it->first;
        if (StringUtils::startsWith(name, metatata_prefix))
        {
          name = name.substr(metatata_prefix.length());

          //trick: azure does not allow the "-" 
          if (StringUtils::contains(name, "_"))
            name = StringUtils::replaceAll(name, "_", "-");

          blob.metadata.setValue(name, it->second);
        }
      }

      blob.body = response.body;

      auto content_type = response.getContentType();
      if (!content_type.empty())
        blob.content_type = content_type;

      ret.get_promise()->set_value(blob);
    });

    return ret;
  }


  // deleteBlob
  virtual Future<bool> deleteBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted) override
  {
    auto ret = Promise<bool>().get_future();

    NetRequest request(this->url.toString() + blob_name, "DELETE");
    request.aborted = aborted;
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }

};

}//namespace

#endif //VISUS_AZURE_CLOUD_STORAGE_

